// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <types/db.h>
#include <types/transaction.h>
#include <util/ffi.h>

#include <format>
#include <string>
#include <string_view>

namespace grovedb {
Db::Db() = default;
Db::~Db() = default;

Db::Db(Db&&) = default;
Db& Db::operator=(Db&&) = default;

Result<Db, Error> Db::Open(std::string_view path)
{
  return CallFFI([&]() -> Result<Db, Error> {
    Db db;
    db.m_impl =
        std::make_unique<Impl>(grovedb_cxx::grovedb_open(rust::Str(path.data(), path.size())));
    return db;
  });
}

Result<void, Error> Db::Flush()
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<void, Error> {
    grovedb_cxx::grovedb_flush(*m_impl->m_db);
    return {};
  });
}

Result<void, Error> Db::Destroy()
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<void, Error> {
    grovedb_cxx::grovedb_wipe(*m_impl->m_db);
    return {};
  });
}

Result<Costed<Hash>, Error> Db::GetRootHash()
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Costed<Hash>, Error> {
    auto result = grovedb_cxx::grovedb_root_hash(*m_impl->m_db);
    auto hash = HashFromSlice(result.root_hash);
    if (!hash) {
      return Err(std::move(hash).error());
    }
    return Costed<Hash>{*hash, convert_cost(result.cost)};
  });
}

Result<bool, Error> Db::VerifyIntegrity()
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<bool, Error> {
    return grovedb_cxx::grovedb_verify(*m_impl->m_db);
  });
}

Result<Transaction, Error> Db::BeginTransaction()
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Transaction, Error> {
    auto tx = grovedb_cxx::grovedb_start_transaction(*m_impl->m_db);
    Transaction txn;
    txn.m_impl = std::make_unique<Transaction::Impl>(*m_impl->m_db, std::move(tx));
    return txn;
  });
}

Result<OperationCost, Error> Db::Commit(Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  if (!txn.m_impl) {
    return Err(Error::InvalidArgument("called with uninitialized transaction"));
  }
  if (txn.m_impl->m_committed || txn.m_impl->m_rolled_back) {
    return Err(Error::InvalidArgument("transaction already finalized"));
  }
  return CallFFI([&]() -> Result<OperationCost, Error> {
    // Mark committed before the call -- the Rust side consumes the
    // Box regardless of success or failure.
    txn.m_impl->m_committed = true;
    auto result =
        grovedb_cxx::grovedb_commit_transaction(txn.m_impl->m_db, std::move(txn.m_impl->m_tx));
    return convert_cost(result);
  });
}

Result<void, Error> Db::Rollback(Transaction& txn)
{
  if (!txn.m_impl) {
    return Err(Error::InvalidArgument("called with uninitialized transaction"));
  }
  if (txn.m_impl->m_committed || txn.m_impl->m_rolled_back) {
    return Err(Error::InvalidArgument("transaction already finalized"));
  }
  return CallFFI([&]() -> Result<void, Error> {
    grovedb_cxx::grovedb_rollback_transaction(txn.m_impl->m_db, *txn.m_impl->m_tx);
    txn.m_impl->m_rolled_back = true;
    return {};
  });
}

std::string GetWhoami()
{
  return std::format("libgrovedb uses {}", std::string(grovedb_cxx::whoami()));
}
} // namespace grovedb

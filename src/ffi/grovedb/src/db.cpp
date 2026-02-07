// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <db_internal.h>
#include <types/transaction.h>

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
  try {
    Db db;
    db.m_impl =
        std::make_unique<Impl>(grovedb_cxx::grovedb_open(rust::Str(path.data(), path.size())));
    return db;
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<void, Error> Db::Flush()
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    grovedb_cxx::grovedb_flush(*m_impl->m_db);
    return {};
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<void, Error> Db::Destroy()
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    grovedb_cxx::grovedb_wipe(*m_impl->m_db);
    return {};
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<Costed<Hash>, Error> Db::GetRootHash()
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto result = grovedb_cxx::grovedb_root_hash(*m_impl->m_db);
    if (result.root_hash.size() != 32) {
      return Err(
          Error::Corruption(
              "root hash: expected 32 bytes, got " + std::to_string(result.root_hash.size())
          )
      );
    }
    Hash hash{};
    std::copy(result.root_hash.begin(), result.root_hash.end(), hash.begin());
    return Costed<Hash>{
        .m_value = hash,
        .m_cost = convert_cost(result.cost),
    };
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<bool, Error> Db::VerifyIntegrity()
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    return grovedb_cxx::grovedb_verify(*m_impl->m_db);
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<Transaction, Error> Db::BeginTransaction()
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto tx = grovedb_cxx::grovedb_start_transaction(*m_impl->m_db);
    Transaction txn;
    txn.m_impl = std::make_unique<Transaction::Impl>(*m_impl->m_db, std::move(tx));
    return txn;
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
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
  try {
    // Mark committed before the call -- the Rust side consumes the
    // Box regardless of success or failure.
    txn.m_impl->m_committed = true;
    auto result =
        grovedb_cxx::grovedb_commit_transaction(txn.m_impl->m_db, std::move(txn.m_impl->m_tx));
    return convert_cost(result);
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<void, Error> Db::Rollback(Transaction& txn)
{
  if (!txn.m_impl) {
    return Err(Error::InvalidArgument("called with uninitialized transaction"));
  }
  if (txn.m_impl->m_committed || txn.m_impl->m_rolled_back) {
    return Err(Error::InvalidArgument("transaction already finalized"));
  }
  try {
    grovedb_cxx::grovedb_rollback_transaction(txn.m_impl->m_db, *txn.m_impl->m_tx);
    txn.m_impl->m_rolled_back = true;
    return {};
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

std::string GetWhoami()
{
  return std::format("libgrovedb uses {}", std::string(grovedb_cxx::whoami()));
}
} // namespace grovedb

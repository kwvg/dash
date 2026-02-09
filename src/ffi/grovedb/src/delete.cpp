// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <db_internal.h>
#include <types/transaction.h>
#include <util/ffi.h>

#include <grovedb/wire.h>

namespace grovedb {
// ---------------------------------------------------------------------------
// Delete — unconditional
// ---------------------------------------------------------------------------

Result<OperationCost, Error> Db::Delete(const Path& path, const Bytes& key)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<OperationCost, Error> {
    auto path_buf = wire::Encode(path);
    auto result = grovedb_cxx::grovedb_delete(*m_impl->m_db, ToSlice(path_buf), ToSlice(key));
    return convert_cost(result);
  });
}

Result<OperationCost, Error> Db::Delete(const Path& path, const Bytes& key, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<OperationCost, Error> {
    auto path_buf = wire::Encode(path);
    auto result = grovedb_cxx::grovedb_delete_with_tx(
        *m_impl->m_db, ToSlice(path_buf), ToSlice(key), *txn.m_impl->m_tx
    );
    return convert_cost(result);
  });
}

// ---------------------------------------------------------------------------
// DeleteIfEmpty — delete only if the target is an empty tree
// ---------------------------------------------------------------------------

Result<Costed<bool>, Error> Db::DeleteIfEmpty(const Path& path, const Bytes& key)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Costed<bool>, Error> {
    auto path_buf = wire::Encode(path);
    auto result =
        grovedb_cxx::grovedb_delete_if_empty_tree(*m_impl->m_db, ToSlice(path_buf), ToSlice(key));
    return ConvertBool(result);
  });
}

Result<Costed<bool>, Error>
Db::DeleteIfEmpty(const Path& path, const Bytes& key, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Costed<bool>, Error> {
    auto path_buf = wire::Encode(path);
    auto result = grovedb_cxx::grovedb_delete_if_empty_tree_with_tx(
        *m_impl->m_db, ToSlice(path_buf), ToSlice(key), *txn.m_impl->m_tx
    );
    return ConvertBool(result);
  });
}

// ---------------------------------------------------------------------------
// PruneEmptyAncestors — cascade delete empty ancestors
// ---------------------------------------------------------------------------

Result<Costed<uint32_t>, Error> Db::PruneEmptyAncestors(const Path& path, const Bytes& key)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Costed<uint32_t>, Error> {
    auto path_buf = wire::Encode(path);
    auto result = grovedb_cxx::grovedb_delete_up_tree_while_empty(
        *m_impl->m_db, ToSlice(path_buf), ToSlice(key)
    );
    return ConvertU32(result);
  });
}

Result<Costed<uint32_t>, Error>
Db::PruneEmptyAncestors(const Path& path, const Bytes& key, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Costed<uint32_t>, Error> {
    auto path_buf = wire::Encode(path);
    auto result = grovedb_cxx::grovedb_delete_up_tree_while_empty_with_tx(
        *m_impl->m_db, ToSlice(path_buf), ToSlice(key), *txn.m_impl->m_tx
    );
    return ConvertU32(result);
  });
}

// ---------------------------------------------------------------------------
// Clear — remove all elements in a subtree
// ---------------------------------------------------------------------------

Result<bool, Error> Db::Clear(const Path& path)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<bool, Error> {
    auto path_buf = wire::Encode(path);
    return grovedb_cxx::grovedb_clear_subtree(*m_impl->m_db, ToSlice(path_buf));
  });
}

Result<bool, Error> Db::Clear(const Path& path, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<bool, Error> {
    auto path_buf = wire::Encode(path);
    return grovedb_cxx::grovedb_clear_subtree_with_tx(
        *m_impl->m_db, ToSlice(path_buf), *txn.m_impl->m_tx
    );
  });
}
} // namespace grovedb

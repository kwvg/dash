// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <db_internal.h>
#include <types/transaction.h>

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
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

    auto result = grovedb_cxx::grovedb_delete(*m_impl->m_db, path_slice, key_slice);
    return convert_cost(result);
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<OperationCost, Error> Db::Delete(const Path& path, const Bytes& key, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

    auto result = grovedb_cxx::grovedb_delete_with_tx(
        *m_impl->m_db, path_slice, key_slice, *txn.m_impl->m_tx
    );
    return convert_cost(result);
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

// ---------------------------------------------------------------------------
// DeleteIfEmpty — delete only if the target is an empty tree
// ---------------------------------------------------------------------------

Result<Costed<bool>, Error> Db::DeleteIfEmpty(const Path& path, const Bytes& key)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

    auto result = grovedb_cxx::grovedb_delete_if_empty_tree(*m_impl->m_db, path_slice, key_slice);
    return Costed<bool>{
        .m_value = result.value,
        .m_cost = convert_cost(result.cost),
    };
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<Costed<bool>, Error>
Db::DeleteIfEmpty(const Path& path, const Bytes& key, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

    auto result = grovedb_cxx::grovedb_delete_if_empty_tree_with_tx(
        *m_impl->m_db, path_slice, key_slice, *txn.m_impl->m_tx
    );
    return Costed<bool>{
        .m_value = result.value,
        .m_cost = convert_cost(result.cost),
    };
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

// ---------------------------------------------------------------------------
// PruneEmptyAncestors — cascade delete empty ancestors
// ---------------------------------------------------------------------------

Result<Costed<uint32_t>, Error> Db::PruneEmptyAncestors(const Path& path, const Bytes& key)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

    auto result =
        grovedb_cxx::grovedb_delete_up_tree_while_empty(*m_impl->m_db, path_slice, key_slice);
    return Costed<uint32_t>{
        .m_value = result.value,
        .m_cost = convert_cost(result.cost),
    };
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<Costed<uint32_t>, Error>
Db::PruneEmptyAncestors(const Path& path, const Bytes& key, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

    auto result = grovedb_cxx::grovedb_delete_up_tree_while_empty_with_tx(
        *m_impl->m_db, path_slice, key_slice, *txn.m_impl->m_tx
    );
    return Costed<uint32_t>{
        .m_value = result.value,
        .m_cost = convert_cost(result.cost),
    };
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

// ---------------------------------------------------------------------------
// Clear — remove all elements in a subtree
// ---------------------------------------------------------------------------

Result<bool, Error> Db::Clear(const Path& path)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};

    return grovedb_cxx::grovedb_clear_subtree(*m_impl->m_db, path_slice);
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<bool, Error> Db::Clear(const Path& path, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};

    return grovedb_cxx::grovedb_clear_subtree_with_tx(*m_impl->m_db, path_slice, *txn.m_impl->m_tx);
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}
} // namespace grovedb

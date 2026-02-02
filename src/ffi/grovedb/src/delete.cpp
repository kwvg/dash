// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifdef HAVE_GROVEDB_CONFIG
#include <config/grovedb-config.h>
#endif

#include "db_internal.h"

#include <grovedb/wire.h>

#include <types/transaction.h>

#include <util/assert.h>

namespace grovedb {

// ---------------------------------------------------------------------------
// Delete operations
// ---------------------------------------------------------------------------

Status Db::Delete(const Path& path, const Bytes& key, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

        auto result = grovedb_cxx::grovedb_delete(*m_impl->m_db, path_slice, key_slice);
        cost = convert_cost(result);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::Delete(const Path& path, const Bytes& key, const Transaction& txn, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

        auto result = grovedb_cxx::grovedb_delete_with_tx(
            *m_impl->m_db, path_slice, key_slice, *txn.m_impl->m_tx);
        cost = convert_cost(result);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::DeleteIfEmpty(const Path& path, const Bytes& key, bool& deleted, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

        auto result = grovedb_cxx::grovedb_delete_if_empty_tree(*m_impl->m_db, path_slice, key_slice);
        deleted = result.value;
        cost = convert_cost(result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::DeleteIfEmpty(const Path& path, const Bytes& key, const Transaction& txn, bool& deleted, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

        auto result = grovedb_cxx::grovedb_delete_if_empty_tree_with_tx(
            *m_impl->m_db, path_slice, key_slice, *txn.m_impl->m_tx);
        deleted = result.value;
        cost = convert_cost(result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::PruneEmptyAncestors(const Path& path, const Bytes& key, uint32_t& removed_count, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

        auto result = grovedb_cxx::grovedb_delete_up_tree_while_empty(
            *m_impl->m_db, path_slice, key_slice);
        removed_count = result.value;
        cost = convert_cost(result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::PruneEmptyAncestors(const Path& path, const Bytes& key, const Transaction& txn, uint32_t& removed_count, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

        auto result = grovedb_cxx::grovedb_delete_up_tree_while_empty_with_tx(
            *m_impl->m_db, path_slice, key_slice, *txn.m_impl->m_tx);
        removed_count = result.value;
        cost = convert_cost(result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::Clear(const Path& path, bool& result)
{
    Assert(m_impl, "called on uninitialized database");
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};

        result = grovedb_cxx::grovedb_clear_subtree(*m_impl->m_db, path_slice);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::Clear(const Path& path, const Transaction& txn, bool& result)
{
    Assert(m_impl, "called on uninitialized database");
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};

        result = grovedb_cxx::grovedb_clear_subtree_with_tx(
            *m_impl->m_db, path_slice, *txn.m_impl->m_tx);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

} // namespace grovedb

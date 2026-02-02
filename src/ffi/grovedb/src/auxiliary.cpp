// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifdef HAVE_GROVEDB_CONFIG
#include <config/grovedb-config.h>
#endif

#include "db_internal.h"

#include <grovedb/wire.h>

#include <types/transaction.h>

namespace grovedb {

// ---------------------------------------------------------------------------
// Auxiliary data operations
// ---------------------------------------------------------------------------

Status Db::PutAux(const Bytes& key, const Bytes& value, OperationCost& cost)
{
    try {
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};
        rust::Slice<const uint8_t> value_slice{value.data(), value.size()};

        auto result = grovedb_cxx::grovedb_put_aux(*m_impl->m_db, key_slice, value_slice);
        cost = convert_cost(result);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::PutAux(const Bytes& key, const Bytes& value, const Transaction& txn, OperationCost& cost)
{
    try {
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};
        rust::Slice<const uint8_t> value_slice{value.data(), value.size()};

        auto result = grovedb_cxx::grovedb_put_aux_with_tx(
            *m_impl->m_db, key_slice, value_slice, *txn.m_impl->m_tx);
        cost = convert_cost(result);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::GetAux(const Bytes& key, std::optional<Bytes>& value, OperationCost& cost)
{
    try {
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

        auto result = grovedb_cxx::grovedb_get_aux(*m_impl->m_db, key_slice);
        cost = convert_cost(result.cost);
        if (result.has_value) {
            value = Bytes(result.value.begin(), result.value.end());
        } else {
            value = std::nullopt;
        }
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::GetAux(const Bytes& key, const Transaction& txn, std::optional<Bytes>& value, OperationCost& cost)
{
    try {
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

        auto result = grovedb_cxx::grovedb_get_aux_with_tx(
            *m_impl->m_db, key_slice, *txn.m_impl->m_tx);
        cost = convert_cost(result.cost);
        if (result.has_value) {
            value = Bytes(result.value.begin(), result.value.end());
        } else {
            value = std::nullopt;
        }
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::DeleteAux(const Bytes& key, OperationCost& cost)
{
    try {
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

        auto result = grovedb_cxx::grovedb_delete_aux(*m_impl->m_db, key_slice);
        cost = convert_cost(result);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::DeleteAux(const Bytes& key, const Transaction& txn, OperationCost& cost)
{
    try {
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

        auto result = grovedb_cxx::grovedb_delete_aux_with_tx(
            *m_impl->m_db, key_slice, *txn.m_impl->m_tx);
        cost = convert_cost(result);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

// ---------------------------------------------------------------------------
// Find subtrees
// ---------------------------------------------------------------------------

Status Db::DecodePaths(
    std::span<const uint8_t> data,
    std::vector<Path>& paths)
{
    wire::Reader r{data};
    uint32_t count{0};
    if (auto s = r.U32(count); !s.ok()) return s;

    paths.clear();
    paths.reserve(count);
    for (uint32_t i{0}; i < count; ++i) {
        Path path;
        if (auto s = wire::WireRead(r, path); !s.ok()) return s;
        paths.push_back(std::move(path));
    }
    return Status::Ok();
}

Status Db::FindSubtrees(const Path& path, std::vector<Path>& subtrees, OperationCost& cost)
{
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};

        auto result = grovedb_cxx::grovedb_find_subtrees(*m_impl->m_db, path_slice);
        cost = convert_cost(result.cost);
        return DecodePaths(
            {result.paths.data(), result.paths.size()},
            subtrees);
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::FindSubtrees(const Path& path, const Transaction& txn, std::vector<Path>& subtrees, OperationCost& cost)
{
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};

        auto result = grovedb_cxx::grovedb_find_subtrees_with_tx(
            *m_impl->m_db, path_slice, *txn.m_impl->m_tx);
        cost = convert_cost(result.cost);
        return DecodePaths(
            {result.paths.data(), result.paths.size()},
            subtrees);
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

} // namespace grovedb

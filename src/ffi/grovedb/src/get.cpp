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
// Get operations
// ---------------------------------------------------------------------------

Status Db::Get(const Path& path, const Bytes& key, Element& element, OperationCost& cost)
{
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

        auto result = grovedb_cxx::grovedb_get(*m_impl->m_db, path_slice, key_slice);
        element.m_data.assign(result.element.begin(), result.element.end());
        cost = convert_cost(result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::Get(const Path& path, const Bytes& key, const Transaction& txn, Element& element, OperationCost& cost)
{
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

        auto result = grovedb_cxx::grovedb_get_with_tx(
            *m_impl->m_db, path_slice, key_slice, *txn.m_impl->m_tx);
        element.m_data.assign(result.element.begin(), result.element.end());
        cost = convert_cost(result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::GetDirect(const Path& path, const Bytes& key, Element& element, OperationCost& cost)
{
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

        auto result = grovedb_cxx::grovedb_get_raw(*m_impl->m_db, path_slice, key_slice);
        element.m_data.assign(result.element.begin(), result.element.end());
        cost = convert_cost(result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::GetDirect(const Path& path, const Bytes& key, const Transaction& txn, Element& element, OperationCost& cost)
{
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

        auto result = grovedb_cxx::grovedb_get_raw_with_tx(
            *m_impl->m_db, path_slice, key_slice, *txn.m_impl->m_tx);
        element.m_data.assign(result.element.begin(), result.element.end());
        cost = convert_cost(result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::GetOptional(const Path& path, const Bytes& key, std::optional<Element>& element, OperationCost& cost)
{
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

        auto result = grovedb_cxx::grovedb_get_raw_optional(*m_impl->m_db, path_slice, key_slice);
        cost = convert_cost(result.cost);
        if (result.has_element) {
            Element elem;
            elem.m_data.assign(result.element.begin(), result.element.end());
            element = std::move(elem);
        } else {
            element = std::nullopt;
        }
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::GetOptional(const Path& path, const Bytes& key, const Transaction& txn, std::optional<Element>& element, OperationCost& cost)
{
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

        auto result = grovedb_cxx::grovedb_get_raw_optional_with_tx(
            *m_impl->m_db, path_slice, key_slice, *txn.m_impl->m_tx);
        cost = convert_cost(result.cost);
        if (result.has_element) {
            Element elem;
            elem.m_data.assign(result.element.begin(), result.element.end());
            element = std::move(elem);
        } else {
            element = std::nullopt;
        }
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::KeyExists(const Path& path, const Bytes& key, bool& result, OperationCost& cost)
{
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

        auto ffi_result = grovedb_cxx::grovedb_has_raw(*m_impl->m_db, path_slice, key_slice);
        result = ffi_result.value;
        cost = convert_cost(ffi_result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::KeyExists(const Path& path, const Bytes& key, const Transaction& txn, bool& result, OperationCost& cost)
{
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

        auto ffi_result = grovedb_cxx::grovedb_has_raw_with_tx(
            *m_impl->m_db, path_slice, key_slice, *txn.m_impl->m_tx);
        result = ffi_result.value;
        cost = convert_cost(ffi_result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::SubtreeExists(const Path& path, bool& result, OperationCost& cost)
{
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};

        auto ffi_result = grovedb_cxx::grovedb_check_subtree_exists(
            *m_impl->m_db, path_slice);
        result = ffi_result.value;
        cost = convert_cost(ffi_result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::SubtreeExists(const Path& path, const Transaction& txn, bool& result, OperationCost& cost)
{
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};

        auto ffi_result = grovedb_cxx::grovedb_check_subtree_exists_with_tx(
            *m_impl->m_db, path_slice, *txn.m_impl->m_tx);
        result = ffi_result.value;
        cost = convert_cost(ffi_result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

// ---------------------------------------------------------------------------
// IsEmptyTree
// ---------------------------------------------------------------------------

Status Db::IsEmptyTree(const Path& path, bool& empty, OperationCost& cost)
{
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};

        auto ffi_result = grovedb_cxx::grovedb_is_empty_tree(*m_impl->m_db, path_slice);
        empty = ffi_result.value;
        cost = convert_cost(ffi_result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::IsEmptyTree(const Path& path, const Transaction& txn, bool& empty, OperationCost& cost)
{
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};

        auto ffi_result = grovedb_cxx::grovedb_is_empty_tree_with_tx(
            *m_impl->m_db, path_slice, *txn.m_impl->m_tx);
        empty = ffi_result.value;
        cost = convert_cost(ffi_result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

} // namespace grovedb

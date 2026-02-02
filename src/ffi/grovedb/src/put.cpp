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
// Put operations
// ---------------------------------------------------------------------------

Status Db::Put(const Path& path, const Bytes& key, const Element& element, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};
        rust::Slice<const uint8_t> elem_slice{element.m_data.data(), element.m_data.size()};

        auto result = grovedb_cxx::grovedb_insert(*m_impl->m_db, path_slice, key_slice, elem_slice);
        cost = convert_cost(result);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::Put(const Path& path, const Bytes& key, const Element& element, const Transaction& txn, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};
        rust::Slice<const uint8_t> elem_slice{element.m_data.data(), element.m_data.size()};

        auto result = grovedb_cxx::grovedb_insert_with_tx(
            *m_impl->m_db, path_slice, key_slice, elem_slice, *txn.m_impl->m_tx);
        cost = convert_cost(result);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::PutIfAbsent(const Path& path, const Bytes& key, const Element& element, bool& inserted, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};
        rust::Slice<const uint8_t> elem_slice{element.m_data.data(), element.m_data.size()};

        auto result = grovedb_cxx::grovedb_insert_if_not_exists(
            *m_impl->m_db, path_slice, key_slice, elem_slice);
        inserted = result.value;
        cost = convert_cost(result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::PutIfAbsent(const Path& path, const Bytes& key, const Element& element, const Transaction& txn, bool& inserted, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};
        rust::Slice<const uint8_t> elem_slice{element.m_data.data(), element.m_data.size()};

        auto result = grovedb_cxx::grovedb_insert_if_not_exists_with_tx(
            *m_impl->m_db, path_slice, key_slice, elem_slice, *txn.m_impl->m_tx);
        inserted = result.value;
        cost = convert_cost(result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::PutIfAbsentAndGet(const Path& path, const Bytes& key, const Element& element, std::optional<Element>& existing, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};
        rust::Slice<const uint8_t> elem_slice{element.m_data.data(), element.m_data.size()};

        auto result = grovedb_cxx::grovedb_insert_if_not_exists_return_existing(
            *m_impl->m_db, path_slice, key_slice, elem_slice);
        cost = convert_cost(result.cost);
        if (result.has_element) {
            Element elem;
            elem.m_data.assign(result.element.begin(), result.element.end());
            existing = std::move(elem);
        } else {
            existing = std::nullopt;
        }
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::PutIfAbsentAndGet(const Path& path, const Bytes& key, const Element& element, const Transaction& txn, std::optional<Element>& existing, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};
        rust::Slice<const uint8_t> elem_slice{element.m_data.data(), element.m_data.size()};

        auto result = grovedb_cxx::grovedb_insert_if_not_exists_return_existing_with_tx(
            *m_impl->m_db, path_slice, key_slice, elem_slice, *txn.m_impl->m_tx);
        cost = convert_cost(result.cost);
        if (result.has_element) {
            Element elem;
            elem.m_data.assign(result.element.begin(), result.element.end());
            existing = std::move(elem);
        } else {
            existing = std::nullopt;
        }
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::PutIfChanged(const Path& path, const Bytes& key, const Element& element, bool& changed, std::optional<Element>& previous, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};
        rust::Slice<const uint8_t> elem_slice{element.m_data.data(), element.m_data.size()};

        auto result = grovedb_cxx::grovedb_insert_if_changed_value(
            *m_impl->m_db, path_slice, key_slice, elem_slice);
        changed = result.changed;
        cost = convert_cost(result.cost);
        if (result.has_previous_element) {
            Element elem;
            elem.m_data.assign(result.previous_element.begin(), result.previous_element.end());
            previous = std::move(elem);
        } else {
            previous = std::nullopt;
        }
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::PutIfChanged(const Path& path, const Bytes& key, const Element& element, const Transaction& txn, bool& changed, std::optional<Element>& previous, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
    try {
        auto path_buf = wire::Encode(path);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
        rust::Slice<const uint8_t> key_slice{key.data(), key.size()};
        rust::Slice<const uint8_t> elem_slice{element.m_data.data(), element.m_data.size()};

        auto result = grovedb_cxx::grovedb_insert_if_changed_value_with_tx(
            *m_impl->m_db, path_slice, key_slice, elem_slice, *txn.m_impl->m_tx);
        changed = result.changed;
        cost = convert_cost(result.cost);
        if (result.has_previous_element) {
            Element elem;
            elem.m_data.assign(result.previous_element.begin(), result.previous_element.end());
            previous = std::move(elem);
        } else {
            previous = std::nullopt;
        }
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

} // namespace grovedb

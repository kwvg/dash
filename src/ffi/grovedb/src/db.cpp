// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifdef HAVE_GROVEDB_CONFIG
#include <config/grovedb-config.h>
#endif

#include <grovedb/db.h>
#include <grovedb/wire.h>

#include <rust/grovedb_cxx/lib.h>
#include <types/query.h>
#include <types/transaction.h>

#include <format>
#include <span>
#include <string>

namespace {
/** Convert an FFI operation cost struct to the public C++ type. */
grovedb::OperationCost convert_cost(const grovedb_cxx::FfiOperationCost& ffi)
{
    return grovedb::OperationCost{
        .m_seek_count = ffi.seek_count,
        .m_storage_added_bytes = ffi.storage_added_bytes,
        .m_storage_replaced_bytes = ffi.storage_replaced_bytes,
        .m_storage_removed_bytes = ffi.storage_removed_bytes,
        .m_storage_loaded_bytes = ffi.storage_loaded_bytes,
        .m_hash_node_calls = ffi.hash_node_calls,
    };
}
} // anonymous namespace

namespace grovedb {
struct Db::Impl {
    rust::Box<grovedb_cxx::BoxedGroveDb> m_db;

    explicit Impl(rust::Box<grovedb_cxx::BoxedGroveDb> db) : m_db(std::move(db)) {}
};

Db::Db() = default;
Db::~Db() = default;

Db::Db(Db&&) = default;
Db& Db::operator=(Db&&) = default;

Status Db::Open(const std::string& path, Db& db)
{
    try {
        db.m_impl = std::make_unique<Impl>(grovedb_cxx::grovedb_open(path));
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::Flush()
{
    try {
        grovedb_cxx::grovedb_flush(*m_impl->m_db);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::Destroy()
{
    try {
        grovedb_cxx::grovedb_wipe(*m_impl->m_db);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::GetRootHash(Hash& hash, OperationCost& cost)
{
    try {
        auto result = grovedb_cxx::grovedb_root_hash(*m_impl->m_db);

        std::copy(result.root_hash.begin(), result.root_hash.end(), hash.begin());

        cost = OperationCost{
            .m_seek_count = result.cost.seek_count,
            .m_storage_added_bytes = result.cost.storage_added_bytes,
            .m_storage_replaced_bytes = result.cost.storage_replaced_bytes,
            .m_storage_removed_bytes = result.cost.storage_removed_bytes,
            .m_storage_loaded_bytes = result.cost.storage_loaded_bytes,
            .m_hash_node_calls = result.cost.hash_node_calls,
        };

        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::VerifyIntegrity(bool& result)
{
    try {
        result = grovedb_cxx::grovedb_verify(*m_impl->m_db);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::BeginTransaction(Transaction& txn)
{
    try {
        auto tx = grovedb_cxx::grovedb_start_transaction(*m_impl->m_db);
        txn.m_impl = std::make_unique<Transaction::Impl>(*m_impl->m_db, std::move(tx));
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::Commit(Transaction& txn, OperationCost& cost)
{
    try {
        // Mark committed before the call -- the Rust side consumes the
        // Box regardless of success or failure.
        txn.m_impl->m_committed = true;
        auto result = grovedb_cxx::grovedb_commit_transaction(
            txn.m_impl->m_db, std::move(txn.m_impl->m_tx));

        cost = OperationCost{
            .m_seek_count = result.seek_count,
            .m_storage_added_bytes = result.storage_added_bytes,
            .m_storage_replaced_bytes = result.storage_replaced_bytes,
            .m_storage_removed_bytes = result.storage_removed_bytes,
            .m_storage_loaded_bytes = result.storage_loaded_bytes,
            .m_hash_node_calls = result.hash_node_calls,
        };

        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::Rollback(Transaction& txn)
{
    try {
        grovedb_cxx::grovedb_rollback_transaction(txn.m_impl->m_db, *txn.m_impl->m_tx);
        txn.m_impl->m_rolled_back = true;
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

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
// Put operations
// ---------------------------------------------------------------------------

Status Db::Put(const Path& path, const Bytes& key, const Element& element, OperationCost& cost)
{
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

// ---------------------------------------------------------------------------
// Delete operations
// ---------------------------------------------------------------------------

Status Db::Delete(const Path& path, const Bytes& key, OperationCost& cost)
{
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

// ---------------------------------------------------------------------------
// Query operations
// ---------------------------------------------------------------------------

Status Db::QueryValues(const PathQuery& query, std::vector<Bytes>& values, uint16_t& skipped, OperationCost& cost)
{
    try {
        auto result = grovedb_cxx::grovedb_query_item_value(
            *m_impl->m_db, *query.m_impl->m_query);
        if (auto s = wire::Decode(std::span<const uint8_t>{result.values.data(), result.values.size()}, values); !s.ok()) {
            return s;
        }
        skipped = result.skipped;
        cost = convert_cost(result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::QueryValues(const PathQuery& query, const Transaction& txn, std::vector<Bytes>& values, uint16_t& skipped, OperationCost& cost)
{
    try {
        auto result = grovedb_cxx::grovedb_query_item_value_with_tx(
            *m_impl->m_db, *query.m_impl->m_query, *txn.m_impl->m_tx);
        if (auto s = wire::Decode(std::span<const uint8_t>{result.values.data(), result.values.size()}, values); !s.ok()) {
            return s;
        }
        skipped = result.skipped;
        cost = convert_cost(result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

// ---------------------------------------------------------------------------
// Proof operations
// ---------------------------------------------------------------------------

Status Db::Prove(const PathQuery& query, Bytes& proof, OperationCost& cost)
{
    return Prove(query, ProveOptions{}, proof, cost);
}

Status Db::Prove(const PathQuery& query, const ProveOptions& options,
                 Bytes& proof, OperationCost& cost)
{
    try {
        auto result = grovedb_cxx::grovedb_prove_query(
            *m_impl->m_db, *query.m_impl->m_query,
            options.m_decrease_limit_on_empty);
        proof.assign(result.proof.begin(), result.proof.end());
        cost = convert_cost(result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::DecodeVerifyResult(
    std::span<const uint8_t> root_hash_bytes,
    std::span<const uint8_t> entries_bytes,
    Hash& root_hash,
    std::vector<ProofResultEntry>& entries)
{
    if (root_hash_bytes.size() != 32) {
        return Status::Corruption("proof: root hash is not 32 bytes");
    }
    std::copy(root_hash_bytes.begin(), root_hash_bytes.end(), root_hash.begin());

    wire::Reader r{entries_bytes};
    uint32_t count{0};
    if (auto s = r.U32(count); !s.ok()) return s;

    entries.clear();
    entries.reserve(count);
    for (uint32_t i{0}; i < count; ++i) {
        ProofResultEntry entry;
        if (auto s = wire::WireRead(r, entry.m_path); !s.ok()) return s;
        if (auto s = r.Bytes(entry.m_key); !s.ok()) return s;

        uint8_t has_elem{0};
        if (auto s = r.U8(has_elem); !s.ok()) return s;
        if (has_elem) {
            Bytes elem_bytes;
            if (auto s = r.Bytes(elem_bytes); !s.ok()) return s;
            Element elem;
            elem.m_data = std::move(elem_bytes);
            entry.m_element = std::move(elem);
        }

        entries.push_back(std::move(entry));
    }
    return Status::Ok();
}

Status Db::VerifyQuery(
    const Bytes& proof, const PathQuery& query,
    Hash& root_hash, std::vector<ProofResultEntry>& entries)
{
    try {
        rust::Slice<const uint8_t> proof_slice{proof.data(), proof.size()};
        auto result = grovedb_cxx::grovedb_verify_query(
            proof_slice, *query.m_impl->m_query);
        return DecodeVerifyResult(
            {result.root_hash.data(), result.root_hash.size()},
            {result.entries.data(), result.entries.size()},
            root_hash, entries);
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::VerifyQuery(
    const Bytes& proof, const PathQuery& query,
    const VerifyOptions& options,
    Hash& root_hash, std::vector<ProofResultEntry>& entries)
{
    try {
        rust::Slice<const uint8_t> proof_slice{proof.data(), proof.size()};
        auto result = grovedb_cxx::grovedb_verify_query_with_options(
            proof_slice, *query.m_impl->m_query,
            options.m_absence_proofs,
            options.m_verify_succinctness,
            options.m_include_empty_trees);
        return DecodeVerifyResult(
            {result.root_hash.data(), result.root_hash.size()},
            {result.entries.data(), result.entries.size()},
            root_hash, entries);
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::VerifySubsetQuery(
    const Bytes& proof, const PathQuery& query,
    Hash& root_hash, std::vector<ProofResultEntry>& entries)
{
    try {
        rust::Slice<const uint8_t> proof_slice{proof.data(), proof.size()};
        auto result = grovedb_cxx::grovedb_verify_subset_query(
            proof_slice, *query.m_impl->m_query);
        return DecodeVerifyResult(
            {result.root_hash.data(), result.root_hash.size()},
            {result.entries.data(), result.entries.size()},
            root_hash, entries);
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::VerifyQueryWithAbsenceProof(
    const Bytes& proof, const PathQuery& query,
    Hash& root_hash, std::vector<ProofResultEntry>& entries)
{
    try {
        rust::Slice<const uint8_t> proof_slice{proof.data(), proof.size()};
        auto result = grovedb_cxx::grovedb_verify_query_with_absence_proof(
            proof_slice, *query.m_impl->m_query);
        return DecodeVerifyResult(
            {result.root_hash.data(), result.root_hash.size()},
            {result.entries.data(), result.entries.size()},
            root_hash, entries);
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::VerifySubsetQueryWithAbsenceProof(
    const Bytes& proof, const PathQuery& query,
    Hash& root_hash, std::vector<ProofResultEntry>& entries)
{
    try {
        rust::Slice<const uint8_t> proof_slice{proof.data(), proof.size()};
        auto result = grovedb_cxx::grovedb_verify_subset_query_with_absence_proof(
            proof_slice, *query.m_impl->m_query);
        return DecodeVerifyResult(
            {result.root_hash.data(), result.root_hash.size()},
            {result.entries.data(), result.entries.size()},
            root_hash, entries);
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

// ---------------------------------------------------------------------------
// Batch operations
// ---------------------------------------------------------------------------

BatchOperation BatchOperation::InsertOnly(Path path, Bytes key, Element element)
{
    BatchOperation op;
    op.m_path = std::move(path);
    op.m_key = std::move(key);
    op.m_kind = Kind::kInsertOnly;
    op.m_element = std::move(element);
    return op;
}

BatchOperation BatchOperation::InsertOrReplace(Path path, Bytes key, Element element)
{
    BatchOperation op;
    op.m_path = std::move(path);
    op.m_key = std::move(key);
    op.m_kind = Kind::kInsertOrReplace;
    op.m_element = std::move(element);
    return op;
}

BatchOperation BatchOperation::Replace(Path path, Bytes key, Element element)
{
    BatchOperation op;
    op.m_path = std::move(path);
    op.m_key = std::move(key);
    op.m_kind = Kind::kReplace;
    op.m_element = std::move(element);
    return op;
}

BatchOperation BatchOperation::Delete(Path path, Bytes key)
{
    BatchOperation op;
    op.m_path = std::move(path);
    op.m_key = std::move(key);
    op.m_kind = Kind::kDelete;
    return op;
}

BatchOperation BatchOperation::DeleteTree(Path path, Bytes key, TreeType tree_type)
{
    BatchOperation op;
    op.m_path = std::move(path);
    op.m_key = std::move(key);
    op.m_kind = Kind::kDeleteTree;
    op.m_tree_type = tree_type;
    return op;
}

Bytes Db::EncodeBatchOps(const std::vector<BatchOperation>& ops)
{
    wire::Writer w;
    w.U32(static_cast<uint32_t>(ops.size()));
    for (const auto& op : ops) {
        wire::WireWrite(w, op.m_path);
        w.Bytes(op.m_key);
        w.U8(static_cast<uint8_t>(op.m_kind));
        switch (op.m_kind) {
        case BatchOperation::Kind::kInsertOnly:
        case BatchOperation::Kind::kInsertOrReplace:
        case BatchOperation::Kind::kReplace:
            w.Bytes(op.m_element.m_data);
            break;
        case BatchOperation::Kind::kDeleteTree:
            w.U8(static_cast<uint8_t>(op.m_tree_type));
            break;
        case BatchOperation::Kind::kDelete:
            break;
        }
    }
    return w.Take();
}

Status Db::ApplyBatch(const std::vector<BatchOperation>& ops,
                      OperationCost& cost)
{
    return ApplyBatch(ops, BatchApplyOptions{}, cost);
}

Status Db::ApplyBatch(const std::vector<BatchOperation>& ops,
                      const BatchApplyOptions& options,
                      OperationCost& cost)
{
    try {
        auto buf = EncodeBatchOps(ops);
        rust::Slice<const uint8_t> ops_slice{buf.data(), buf.size()};
        grovedb_cxx::FfiBatchApplyOptions ffi_opts{
            options.m_validate_insertion_does_not_override,
            options.m_validate_insertion_does_not_override_tree,
            options.m_allow_deleting_non_empty_trees,
            options.m_deleting_non_empty_trees_returns_error,
            options.m_disable_operation_consistency_check,
            options.m_base_root_storage_is_free,
        };
        auto result = grovedb_cxx::grovedb_apply_batch(
            *m_impl->m_db, ops_slice, ffi_opts);
        cost = convert_cost(result);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::ApplyBatch(const std::vector<BatchOperation>& ops,
                      const Transaction& txn,
                      OperationCost& cost)
{
    return ApplyBatch(ops, BatchApplyOptions{}, txn, cost);
}

Status Db::ApplyBatch(const std::vector<BatchOperation>& ops,
                      const BatchApplyOptions& options,
                      const Transaction& txn,
                      OperationCost& cost)
{
    try {
        auto buf = EncodeBatchOps(ops);
        rust::Slice<const uint8_t> ops_slice{buf.data(), buf.size()};
        grovedb_cxx::FfiBatchApplyOptions ffi_opts{
            options.m_validate_insertion_does_not_override,
            options.m_validate_insertion_does_not_override_tree,
            options.m_allow_deleting_non_empty_trees,
            options.m_deleting_non_empty_trees_returns_error,
            options.m_disable_operation_consistency_check,
            options.m_base_root_storage_is_free,
        };
        auto result = grovedb_cxx::grovedb_apply_batch_with_tx(
            *m_impl->m_db, ops_slice, ffi_opts, *txn.m_impl->m_tx);
        cost = convert_cost(result);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

std::string GetWhoami()
{
    return std::format("{} {} uses {}", PACKAGE_NAME, PACKAGE_VERSION, std::string(grovedb_cxx::whoami()));
}
} // namespace grovedb

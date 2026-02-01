// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifdef HAVE_GROVEDB_CONFIG
#include <config/grovedb-config.h>
#endif

#include <grovedb/db.h>

#include <rust/grovedb_cxx/lib.h>
#include <types/transaction.h>

#include <algorithm>
#include <format>
#include <string>

namespace {
/** Encode a grovedb::Path into the flat wire format expected by the CXX bridge.
 *
 *  Layout (little-endian u32):
 *  [segment_count][len₁][bytes₁][len₂][bytes₂]…
 */
std::vector<uint8_t> encode_path(const grovedb::Path& path)
{
    size_t total{4};
    for (const auto& seg : path) total += 4 + seg.size();

    std::vector<uint8_t> buf;
    buf.reserve(total);

    auto push_u32 = [&buf](uint32_t v) {
        buf.push_back(static_cast<uint8_t>(v));
        buf.push_back(static_cast<uint8_t>(v >> 8));
        buf.push_back(static_cast<uint8_t>(v >> 16));
        buf.push_back(static_cast<uint8_t>(v >> 24));
    };

    push_u32(static_cast<uint32_t>(path.size()));
    for (const auto& seg : path) {
        push_u32(static_cast<uint32_t>(seg.size()));
        buf.insert(buf.end(), seg.begin(), seg.end());
    }

    return buf;
}

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
        auto path_buf = encode_path(path);
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
        auto path_buf = encode_path(path);
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
        auto path_buf = encode_path(path);
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
        auto path_buf = encode_path(path);
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
        auto path_buf = encode_path(path);
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
        auto path_buf = encode_path(path);
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
        auto path_buf = encode_path(path);
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
        auto path_buf = encode_path(path);
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
        auto path_buf = encode_path(path);
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
        auto path_buf = encode_path(path);
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

std::string GetWhoami()
{
    return std::format("{} {} uses {}", PACKAGE_NAME, PACKAGE_VERSION, std::string(grovedb_cxx::whoami()));
}
} // namespace grovedb

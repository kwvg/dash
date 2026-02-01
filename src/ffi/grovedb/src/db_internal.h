// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GROVEDB_DB_INTERNAL_H
#define GROVEDB_DB_INTERNAL_H

#include <grovedb/db.h>

#include <rust/grovedb_cxx/lib.h>

#include <utility>

namespace grovedb {

struct Db::Impl {
    rust::Box<grovedb_cxx::BoxedGroveDb> m_db;

    explicit Impl(rust::Box<grovedb_cxx::BoxedGroveDb> db) : m_db(std::move(db)) {}
};

/** Convert an FFI operation cost struct to the public C++ type. */
inline OperationCost convert_cost(const grovedb_cxx::FfiOperationCost& ffi)
{
    return OperationCost{
        .m_seek_count = ffi.seek_count,
        .m_storage_added_bytes = ffi.storage_added_bytes,
        .m_storage_replaced_bytes = ffi.storage_replaced_bytes,
        .m_storage_removed_bytes = ffi.storage_removed_bytes,
        .m_storage_loaded_bytes = ffi.storage_loaded_bytes,
        .m_hash_node_calls = ffi.hash_node_calls,
    };
}

} // namespace grovedb

#endif // GROVEDB_DB_INTERNAL_H

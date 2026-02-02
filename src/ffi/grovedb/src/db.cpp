// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifdef HAVE_GROVEDB_CONFIG
#include <config/grovedb-config.h>
#endif

#include "db_internal.h"

#include <types/transaction.h>

#include <util/assert.h>

#include <format>
#include <string>

#ifndef PACKAGE_NAME
#define PACKAGE_NAME "libgrovedb"
#endif
#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "unknown"
#endif

namespace grovedb {

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
    Assert(m_impl, "called on uninitialized database");
    try {
        grovedb_cxx::grovedb_flush(*m_impl->m_db);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::Destroy()
{
    Assert(m_impl, "called on uninitialized database");
    try {
        grovedb_cxx::grovedb_wipe(*m_impl->m_db);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::GetRootHash(Hash& hash, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
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
    Assert(m_impl, "called on uninitialized database");
    try {
        result = grovedb_cxx::grovedb_verify(*m_impl->m_db);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::BeginTransaction(Transaction& txn)
{
    Assert(m_impl, "called on uninitialized database");
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
    Assert(m_impl, "called on uninitialized database");
    Assert(txn.m_impl, "called with uninitialized transaction");
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
    Assert(txn.m_impl, "called with uninitialized transaction");
    try {
        grovedb_cxx::grovedb_rollback_transaction(txn.m_impl->m_db, *txn.m_impl->m_tx);
        txn.m_impl->m_rolled_back = true;
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

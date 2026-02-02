// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifdef HAVE_GROVEDB_CONFIG
#include <config/grovedb-config.h>
#endif

#include "db_internal.h"

#include <util/assert.h>

namespace grovedb {

// ---------------------------------------------------------------------------
// Checkpoint operations
// ---------------------------------------------------------------------------

Status Db::CreateCheckpoint(const std::string& path)
{
    Assert(m_impl, "called on uninitialized database");
    try {
        grovedb_cxx::grovedb_create_checkpoint(*m_impl->m_db, path);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::OpenCheckpoint(const std::string& path, Db& db)
{
    try {
        auto boxed = grovedb_cxx::grovedb_open_checkpoint(path);
        db.m_impl = std::make_unique<Impl>(std::move(boxed));
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::DeleteCheckpoint(const std::string& path)
{
    try {
        grovedb_cxx::grovedb_delete_checkpoint(path);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

} // namespace grovedb

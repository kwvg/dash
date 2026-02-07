// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <db_internal.h>

#include <rust/grovedb_cxx/lib.h>

#include <string_view>
#include <utility>

namespace grovedb {
// ---------------------------------------------------------------------------
// CreateCheckpoint
// ---------------------------------------------------------------------------

Result<void, Error> Db::CreateCheckpoint(std::string_view path)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    grovedb_cxx::grovedb_create_checkpoint(*m_impl->m_db, rust::Str(path.data(), path.size()));
    return {};
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

// ---------------------------------------------------------------------------
// OpenCheckpoint
// ---------------------------------------------------------------------------

Result<Db, Error> Db::OpenCheckpoint(std::string_view path)
{
  try {
    auto boxed = grovedb_cxx::grovedb_open_checkpoint(rust::Str(path.data(), path.size()));
    Db db;
    db.m_impl = std::make_unique<Impl>(std::move(boxed));
    return db;
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

// ---------------------------------------------------------------------------
// DeleteCheckpoint
// ---------------------------------------------------------------------------

Result<void, Error> Db::DeleteCheckpoint(std::string_view path)
{
  try {
    grovedb_cxx::grovedb_delete_checkpoint(rust::Str(path.data(), path.size()));
    return {};
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}
} // namespace grovedb

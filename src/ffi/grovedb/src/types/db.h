// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#ifndef GROVEDB_TYPES_DB_H
#define GROVEDB_TYPES_DB_H

#include <grovedb/db.h>

#include <rust/grovedb_cxx/lib.h>

#include <utility>

namespace grovedb {
/// Owns the opaque CXX bridge type that backs a grovedb::Db.
struct Db::Impl {
  rust::Box<grovedb_cxx::BoxedGroveDb> m_db;

  explicit Impl(rust::Box<grovedb_cxx::BoxedGroveDb> db)
      : m_db(std::move(db))
  {
  }
};
} // namespace grovedb

#endif // GROVEDB_TYPES_DB_H

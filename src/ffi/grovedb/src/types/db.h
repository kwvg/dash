// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#ifndef GROVEDB_TYPES_DB_H
#define GROVEDB_TYPES_DB_H

#include <grovedb/db.h>

#include <rust/grovedb_cxx/lib.h>

#include <memory>
#include <utility>

namespace grovedb {
/// Owns the opaque CXX bridge type that backs a grovedb::Db.
///
/// The BoxedGroveDb is held via shared_ptr so that Transaction::Impl
/// can share ownership, preventing a dangling reference if the Db is
/// destroyed while a Transaction still exists.
struct Db::Impl {
  std::shared_ptr<rust::Box<grovedb_cxx::BoxedGroveDb>> m_db;

  explicit Impl(rust::Box<grovedb_cxx::BoxedGroveDb> db)
      : m_db(std::make_shared<rust::Box<grovedb_cxx::BoxedGroveDb>>(std::move(db)))
  {
  }
};
} // namespace grovedb

#endif // GROVEDB_TYPES_DB_H

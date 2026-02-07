// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#ifndef GROVEDB_TYPES_QUERY_H
#define GROVEDB_TYPES_QUERY_H

#include <grovedb/query.h>

#include <rust/grovedb_cxx/lib.h>

#include <utility>

namespace grovedb {
/// Owns the opaque CXX bridge type that backs a grovedb::PathQuery.
struct PathQuery::Impl {
  rust::Box<grovedb_cxx::BoxedPathQuery> m_query;

  explicit Impl(rust::Box<grovedb_cxx::BoxedPathQuery> q)
      : m_query(std::move(q))
  {
  }
};
} // namespace grovedb

#endif // GROVEDB_TYPES_QUERY_H

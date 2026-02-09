// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#ifndef GROVEDB_TYPES_TRANSACTION_H
#define GROVEDB_TYPES_TRANSACTION_H

#include <grovedb/transaction.h>

#include <rust/grovedb_cxx/lib.h>

#include <memory>
#include <utility>

namespace grovedb {
/// Owns the opaque CXX bridge types that back a grovedb::Transaction.
///
/// m_db shares ownership of the BoxedGroveDb with Db::Impl, ensuring
/// the database outlives the transaction even if Db is destroyed first.
struct Transaction::Impl {
  std::shared_ptr<rust::Box<grovedb_cxx::BoxedGroveDb>> m_db;
  rust::Box<grovedb_cxx::BoxedTransaction> m_tx;
  bool m_committed{false};
  bool m_rolled_back{false};

  Impl(std::shared_ptr<rust::Box<grovedb_cxx::BoxedGroveDb>> db,
       rust::Box<grovedb_cxx::BoxedTransaction> tx)
      : m_db(std::move(db))
      , m_tx(std::move(tx))
  {
  }
};
} // namespace grovedb

#endif // GROVEDB_TYPES_TRANSACTION_H

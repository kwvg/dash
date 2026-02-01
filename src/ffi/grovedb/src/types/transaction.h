// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GROVEDB_TYPES_TRANSACTION_H
#define GROVEDB_TYPES_TRANSACTION_H

#include <grovedb/transaction.h>

#include <rust/grovedb_cxx/lib.h>

#include <utility>

namespace grovedb {
// Owns the opaque CXX bridge types that back a grovedb::Transaction.
struct Transaction::Impl {
    grovedb_cxx::BoxedGroveDb& m_db;
    rust::Box<grovedb_cxx::BoxedTransaction> m_tx;
    bool m_committed{false};
    bool m_rolled_back{false};

    Impl(grovedb_cxx::BoxedGroveDb& db, rust::Box<grovedb_cxx::BoxedTransaction> tx)
        : m_db(db), m_tx(std::move(tx)) {}
};
} // namespace grovedb

#endif // GROVEDB_TYPES_TRANSACTION_H

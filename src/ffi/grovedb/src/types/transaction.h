// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#ifndef GROVEDB_TYPES_TRANSACTION_H
#define GROVEDB_TYPES_TRANSACTION_H

#include <grovedb/transaction.h>

#include <rust/grovedb_cxx/lib.h>

#include <utility>

namespace grovedb {
/// Owns the opaque CXX bridge types that back a grovedb::Transaction.
///
/// Populated in Phase 1 when transaction operations are implemented.
/// For now, this is a forward declaration of the pimpl struct.
struct Transaction::Impl {
  // Phase 1 will add: BoxedGroveDb& m_db, Box<BoxedTransaction> m_tx, flags
};
} // namespace grovedb

#endif // GROVEDB_TYPES_TRANSACTION_H

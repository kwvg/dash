// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#ifndef LIBGROVEDB_TRANSACTION_H
#define LIBGROVEDB_TRANSACTION_H

#include <memory>

namespace grovedb {
/** @addtogroup transactions
 *  @{ */
class Db;

/**
 * RAII transaction handle for GroveDB operations.
 *
 * Move-only. If destroyed without an explicit Commit or Rollback,
 * the destructor will attempt to roll back automatically.
 */
class Transaction
{
public:
  /** Construct an empty (unassociated) transaction. */
  Transaction();
  ~Transaction();

  Transaction(const Transaction&) = delete;
  Transaction& operator=(const Transaction&) = delete;
  Transaction(Transaction&&);
  Transaction& operator=(Transaction&&);

private:
  friend class Db;
  struct Impl;
  std::unique_ptr<Impl> m_impl;
};
/** @} */
} // namespace grovedb

#endif // LIBGROVEDB_TRANSACTION_H

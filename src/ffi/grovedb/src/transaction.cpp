// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <types/transaction.h>

namespace grovedb {
Transaction::Transaction() = default;

Transaction::~Transaction()
{
  if (m_impl && !m_impl->m_committed && !m_impl->m_rolled_back) {
    try {
      grovedb_cxx::grovedb_rollback_transaction(**m_impl->m_db, *m_impl->m_tx);
    } catch (...) {
      // Suppress exceptions in destructor.
    }
  }
}

Transaction::Transaction(Transaction&&) = default;
Transaction& Transaction::operator=(Transaction&&) = default;
} // namespace grovedb

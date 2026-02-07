// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <types/transaction.h>

namespace grovedb {
Transaction::Transaction() = default;
Transaction::~Transaction() = default;
Transaction::Transaction(Transaction&&) = default;
Transaction& Transaction::operator=(Transaction&&) = default;
} // namespace grovedb

// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GROVEDB_TRANSACTION_H
#define GROVEDB_TRANSACTION_H

#include <memory>

namespace grovedb {
class Db;

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

} // namespace grovedb

#endif // GROVEDB_TRANSACTION_H

// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// FUZZ TARGET: fuzz_transaction
// Demonstrates: BeginTransaction(), Commit(), Rollback(),
//               Put()/Get() with transaction, GetRootHash()
//

#include <FuzzedDataProvider.h>
#include <utils/check.h>
#include <utils/grovedb.h>
#include <utils/tempdir.h>

#include <grovedb/db.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    grovedb::fuzz::TempDir tmp{"fuzz_transaction"};
    grovedb::Db db;
    if (!grovedb::Db::Open(tmp.path(), db).ok()) return 0;

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    grovedb::Bytes key = grovedb::fuzz::ConsumeKey(fdp);
    grovedb::Bytes value = grovedb::fuzz::ConsumeValue(fdp);
    grovedb::Element item;
    if (!grovedb::Element::Item(value, item).ok()) return 0;

    // Snapshot root hash before.
    grovedb::Hash hash_before{};
    CHECK_OK(db.GetRootHash(hash_before, cost));

    // Begin a transaction and put within it.
    grovedb::Transaction txn;
    CHECK_OK(db.BeginTransaction(txn));
    CHECK_OK(db.Put(root, key, item, txn, cost));

    // Within the txn: visible.
    grovedb::Element txn_fetched;
    CHECK_OK(db.GetDirect(root, key, txn, txn_fetched, cost));
    CHECK_EQ(txn_fetched.data(), item.data());

    // Fuzz-decide: commit or rollback.
    bool do_commit = fdp.ConsumeBool();
    if (do_commit) {
        CHECK_OK(db.Commit(txn, cost));

        // After commit: visible without transaction.
        grovedb::Element committed;
        CHECK_OK(db.GetDirect(root, key, committed, cost));
        CHECK_EQ(committed.data(), item.data());
    } else {
        CHECK_OK(db.Rollback(txn));

        // After rollback: not visible.
        bool exists{false};
        CHECK_OK(db.KeyExists(root, key, exists, cost));
        CHECK_TRUE(!exists);
    }

    // Root hash should be consistent.
    grovedb::Hash hash_after{};
    CHECK_OK(db.GetRootHash(hash_after, cost));
    if (!do_commit) {
        CHECK_EQ(hash_before, hash_after);
    }

    return 0;
}

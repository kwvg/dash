// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <grovedb/db.h>

#include <test/util/tempdir.h>

BOOST_AUTO_TEST_SUITE(transaction_tests)

BOOST_AUTO_TEST_CASE(test_begin_and_commit)
{
    grovedb::test::TempDir tmp{"grovedb_test_tx_commit"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::Transaction txn;
    BOOST_REQUIRE(db.BeginTransaction(txn).ok());

    grovedb::OperationCost cost{};
    BOOST_CHECK(db.Commit(txn, cost).ok());
}

BOOST_AUTO_TEST_CASE(test_explicit_rollback)
{
    grovedb::test::TempDir tmp{"grovedb_test_tx_rollback"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::Transaction txn;
    BOOST_REQUIRE(db.BeginTransaction(txn).ok());

    BOOST_CHECK(db.Rollback(txn).ok());
}

BOOST_AUTO_TEST_CASE(test_auto_rollback_on_destroy)
{
    grovedb::test::TempDir tmp{"grovedb_test_tx_auto_rollback"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    {
        grovedb::Transaction txn;
        BOOST_REQUIRE(db.BeginTransaction(txn).ok());
        // txn goes out of scope without commit or rollback
    }

    // Database should still be usable after auto-rollback.
    grovedb::Hash hash{};
    grovedb::OperationCost cost{};
    auto status = db.GetRootHash(hash, cost);
    BOOST_CHECK(status.ok());
    BOOST_CHECK_EQUAL(hash.size(), 32);
}

BOOST_AUTO_TEST_CASE(test_move_transaction)
{
    grovedb::test::TempDir tmp{"grovedb_test_tx_move"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::Transaction txn;
    BOOST_REQUIRE(db.BeginTransaction(txn).ok());

    auto txn2 = std::move(txn);

    grovedb::OperationCost cost{};
    BOOST_CHECK(db.Commit(txn2, cost).ok());
}

BOOST_AUTO_TEST_SUITE_END()

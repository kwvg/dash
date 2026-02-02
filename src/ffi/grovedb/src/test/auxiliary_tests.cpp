// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <grovedb/db.h>

#include <test/util/tempdir.h>

BOOST_AUTO_TEST_SUITE(auxiliary_tests)

BOOST_AUTO_TEST_CASE(test_put_get_aux_roundtrip)
{
    grovedb::test::TempDir tmp{"grovedb_test_put_get_aux"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Bytes key{'m', 'y', 'k', 'e', 'y'};
    grovedb::Bytes value{'h', 'e', 'l', 'l', 'o'};

    // Store auxiliary data.
    BOOST_REQUIRE(db.PutAux(key, value, cost).ok());

    // Retrieve and verify.
    std::optional<grovedb::Bytes> fetched;
    BOOST_REQUIRE(db.GetAux(key, fetched, cost).ok());
    BOOST_REQUIRE(fetched.has_value());
    BOOST_CHECK(*fetched == value);
}

BOOST_AUTO_TEST_CASE(test_get_aux_nonexistent)
{
    grovedb::test::TempDir tmp{"grovedb_test_get_aux_nonexistent"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Bytes key{'n', 'o', 'p', 'e'};

    std::optional<grovedb::Bytes> fetched;
    BOOST_REQUIRE(db.GetAux(key, fetched, cost).ok());
    BOOST_CHECK(!fetched.has_value());
}

BOOST_AUTO_TEST_CASE(test_delete_aux)
{
    grovedb::test::TempDir tmp{"grovedb_test_delete_aux"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Bytes key{'d', 'e', 'l'};
    grovedb::Bytes value{'v'};

    // Store, then delete.
    BOOST_REQUIRE(db.PutAux(key, value, cost).ok());
    BOOST_REQUIRE(db.DeleteAux(key, cost).ok());

    // Should no longer exist.
    std::optional<grovedb::Bytes> fetched;
    BOOST_REQUIRE(db.GetAux(key, fetched, cost).ok());
    BOOST_CHECK(!fetched.has_value());
}

BOOST_AUTO_TEST_CASE(test_aux_with_transaction)
{
    grovedb::test::TempDir tmp{"grovedb_test_aux_with_tx"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Bytes key{'t', 'x', 'k'};
    grovedb::Bytes value{'t', 'x', 'v'};

    // PutAux within a transaction.
    grovedb::Transaction txn;
    BOOST_REQUIRE(db.BeginTransaction(txn).ok());
    BOOST_REQUIRE(db.PutAux(key, value, txn, cost).ok());

    // Visible within the transaction.
    std::optional<grovedb::Bytes> fetched;
    BOOST_REQUIRE(db.GetAux(key, txn, fetched, cost).ok());
    BOOST_REQUIRE(fetched.has_value());
    BOOST_CHECK(*fetched == value);

    BOOST_REQUIRE(db.Commit(txn, cost).ok());

    // Visible after commit without transaction.
    fetched.reset();
    BOOST_REQUIRE(db.GetAux(key, fetched, cost).ok());
    BOOST_REQUIRE(fetched.has_value());
    BOOST_CHECK(*fetched == value);
}

BOOST_AUTO_TEST_CASE(test_delete_aux_with_transaction)
{
    grovedb::test::TempDir tmp{"grovedb_test_delete_aux_tx"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Bytes key{'d', 'k'};
    grovedb::Bytes value{'d', 'v'};

    // Store outside transaction.
    BOOST_REQUIRE(db.PutAux(key, value, cost).ok());

    // Delete within a transaction.
    grovedb::Transaction txn;
    BOOST_REQUIRE(db.BeginTransaction(txn).ok());
    BOOST_REQUIRE(db.DeleteAux(key, txn, cost).ok());
    BOOST_REQUIRE(db.Commit(txn, cost).ok());

    // Should be gone.
    std::optional<grovedb::Bytes> fetched;
    BOOST_REQUIRE(db.GetAux(key, fetched, cost).ok());
    BOOST_CHECK(!fetched.has_value());
}

BOOST_AUTO_TEST_CASE(test_find_subtrees)
{
    grovedb::test::TempDir tmp{"grovedb_test_find_subtrees"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};

    // Create nested subtrees: root -> s1 -> s1a
    grovedb::Element tree;
    BOOST_REQUIRE(grovedb::Element::EmptyTree(tree).ok());

    grovedb::Bytes s1_key{'s', '1'};
    grovedb::Bytes s1a_key{'s', '1', 'a'};

    BOOST_REQUIRE(db.Put(grovedb::Path{}, s1_key, tree, cost).ok());
    BOOST_REQUIRE(db.Put(grovedb::Path{s1_key}, s1a_key, tree, cost).ok());

    // Find all subtrees from root.
    std::vector<grovedb::Path> subtrees;
    BOOST_REQUIRE(db.FindSubtrees(grovedb::Path{}, subtrees, cost).ok());

    // Should include root itself, s1, and s1/s1a (3 total).
    BOOST_CHECK_EQUAL(subtrees.size(), 3u);
}

BOOST_AUTO_TEST_CASE(test_find_subtrees_with_transaction)
{
    grovedb::test::TempDir tmp{"grovedb_test_find_subtrees_tx"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};

    grovedb::Element tree;
    BOOST_REQUIRE(grovedb::Element::EmptyTree(tree).ok());

    grovedb::Bytes s1_key{'s', '1'};

    // Create subtree within a transaction.
    grovedb::Transaction txn;
    BOOST_REQUIRE(db.BeginTransaction(txn).ok());
    BOOST_REQUIRE(db.Put(grovedb::Path{}, s1_key, tree, txn, cost).ok());

    // Find subtrees within the transaction.
    std::vector<grovedb::Path> subtrees;
    BOOST_REQUIRE(db.FindSubtrees(grovedb::Path{}, txn, subtrees, cost).ok());

    // Should include root and s1 (2 total).
    BOOST_CHECK_EQUAL(subtrees.size(), 2u);

    BOOST_REQUIRE(db.Commit(txn, cost).ok());
}

BOOST_AUTO_TEST_SUITE_END()

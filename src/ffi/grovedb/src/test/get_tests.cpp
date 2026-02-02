// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <grovedb/db.h>

#include <test/util/tempdir.h>

BOOST_AUTO_TEST_SUITE(get_tests)

BOOST_AUTO_TEST_CASE(test_get_nonexistent)
{
    grovedb::test::TempDir tmp{"grovedb_test_get_nonexistent"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::Element element;
    grovedb::OperationCost cost{};
    grovedb::Path path{};
    grovedb::Bytes key{'n', 'o', 'p', 'e'};

    auto status = db.Get(path, key, element, cost);
    BOOST_CHECK(!status.ok());
    BOOST_CHECK(element.empty());
}

BOOST_AUTO_TEST_CASE(test_get_direct_nonexistent)
{
    grovedb::test::TempDir tmp{"grovedb_test_get_direct_nonexistent"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::Element element;
    grovedb::OperationCost cost{};
    grovedb::Path path{};
    grovedb::Bytes key{'n', 'o', 'p', 'e'};

    auto status = db.GetDirect(path, key, element, cost);
    BOOST_CHECK(!status.ok());
    BOOST_CHECK(element.empty());
}

BOOST_AUTO_TEST_CASE(test_get_optional_nonexistent)
{
    grovedb::test::TempDir tmp{"grovedb_test_get_optional_nonexistent"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    std::optional<grovedb::Element> element;
    grovedb::OperationCost cost{};
    grovedb::Path path{};
    grovedb::Bytes key{'n', 'o', 'p', 'e'};

    auto status = db.GetOptional(path, key, element, cost);
    BOOST_CHECK(status.ok());
    BOOST_CHECK(!element.has_value());
}

BOOST_AUTO_TEST_CASE(test_key_exists_nonexistent)
{
    grovedb::test::TempDir tmp{"grovedb_test_key_exists_nonexistent"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    bool exists{true};
    grovedb::OperationCost cost{};
    grovedb::Path path{};
    grovedb::Bytes key{'n', 'o', 'p', 'e'};

    auto status = db.KeyExists(path, key, exists, cost);
    BOOST_CHECK(status.ok());
    BOOST_CHECK(!exists);
}

BOOST_AUTO_TEST_CASE(test_subtree_exists_nonexistent)
{
    grovedb::test::TempDir tmp{"grovedb_test_subtree_exists_nonexistent"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    bool exists{true};
    grovedb::OperationCost cost{};
    grovedb::Path path{{'n', 'o', 'p', 'e'}};

    auto status = db.SubtreeExists(path, exists, cost);
    BOOST_CHECK(status.ok());
    BOOST_CHECK(!exists);
}

BOOST_AUTO_TEST_CASE(test_get_with_transaction)
{
    grovedb::test::TempDir tmp{"grovedb_test_get_with_tx"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::Transaction txn;
    BOOST_REQUIRE(db.BeginTransaction(txn).ok());

    grovedb::Element element;
    grovedb::OperationCost cost{};
    grovedb::Path path{};
    grovedb::Bytes key{'k'};

    // Get within a transaction on an empty db should fail gracefully.
    auto status = db.Get(path, key, txn, element, cost);
    BOOST_CHECK(!status.ok());

    // GetOptional within a transaction should return nullopt.
    std::optional<grovedb::Element> opt_element;
    status = db.GetOptional(path, key, txn, opt_element, cost);
    BOOST_CHECK(status.ok());
    BOOST_CHECK(!opt_element.has_value());

    // KeyExists within a transaction should return false.
    bool exists{true};
    status = db.KeyExists(path, key, txn, exists, cost);
    BOOST_CHECK(status.ok());
    BOOST_CHECK(!exists);

    BOOST_CHECK(db.Rollback(txn).ok());
}

BOOST_AUTO_TEST_CASE(test_is_empty_tree_empty)
{
    grovedb::test::TempDir tmp{"grovedb_test_is_empty_tree_empty"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};

    // Create a subtree at root.
    grovedb::Element tree;
    BOOST_REQUIRE(grovedb::Element::EmptyTree(tree).ok());
    grovedb::Bytes key{'s'};
    BOOST_REQUIRE(db.Put(grovedb::Path{}, key, tree, cost).ok());

    // The subtree should be empty.
    bool empty{false};
    BOOST_REQUIRE(db.IsEmptyTree(grovedb::Path{key}, empty, cost).ok());
    BOOST_CHECK(empty);
}

BOOST_AUTO_TEST_CASE(test_is_empty_tree_not_empty)
{
    grovedb::test::TempDir tmp{"grovedb_test_is_empty_tree_not_empty"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};

    // Create a subtree and put an item in it.
    grovedb::Element tree;
    BOOST_REQUIRE(grovedb::Element::EmptyTree(tree).ok());
    grovedb::Bytes subtree_key{'s'};
    BOOST_REQUIRE(db.Put(grovedb::Path{}, subtree_key, tree, cost).ok());

    grovedb::Element item;
    BOOST_REQUIRE(grovedb::Element::Item(grovedb::Bytes{'v'}, item).ok());
    BOOST_REQUIRE(db.Put(grovedb::Path{subtree_key}, grovedb::Bytes{'k'}, item, cost).ok());

    // The subtree should not be empty.
    bool empty{true};
    BOOST_REQUIRE(db.IsEmptyTree(grovedb::Path{subtree_key}, empty, cost).ok());
    BOOST_CHECK(!empty);
}

BOOST_AUTO_TEST_CASE(test_is_empty_tree_nonexistent)
{
    grovedb::test::TempDir tmp{"grovedb_test_is_empty_tree_nonexistent"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};

    // Non-existent path should return error.
    bool empty{false};
    auto status = db.IsEmptyTree(grovedb::Path{{'n', 'o', 'p', 'e'}}, empty, cost);
    BOOST_CHECK(!status.ok());
}

BOOST_AUTO_TEST_CASE(test_is_empty_tree_with_transaction)
{
    grovedb::test::TempDir tmp{"grovedb_test_is_empty_tree_tx"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};

    // Create a subtree in a transaction.
    grovedb::Element tree;
    BOOST_REQUIRE(grovedb::Element::EmptyTree(tree).ok());
    grovedb::Bytes key{'s'};

    grovedb::Transaction txn;
    BOOST_REQUIRE(db.BeginTransaction(txn).ok());
    BOOST_REQUIRE(db.Put(grovedb::Path{}, key, tree, txn, cost).ok());

    // Check within the transaction — should be empty.
    bool empty{false};
    BOOST_REQUIRE(db.IsEmptyTree(grovedb::Path{key}, txn, empty, cost).ok());
    BOOST_CHECK(empty);

    BOOST_REQUIRE(db.Commit(txn, cost).ok());
}

BOOST_AUTO_TEST_SUITE_END()

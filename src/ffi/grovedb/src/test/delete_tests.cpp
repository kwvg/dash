// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <grovedb/db.h>

#include <test/util/tempdir.h>

BOOST_AUTO_TEST_SUITE(delete_tests)

BOOST_AUTO_TEST_CASE(test_delete_item)
{
    grovedb::test::TempDir tmp{"grovedb_test_delete_item"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path path{};
    grovedb::Bytes key{'k'};

    // Insert an item.
    grovedb::Element item;
    grovedb::Bytes value{'v'};
    BOOST_REQUIRE(grovedb::Element::Item(value, item).ok());
    BOOST_REQUIRE(db.Put(path, key, item, cost).ok());

    // Verify it exists.
    bool exists{false};
    BOOST_REQUIRE(db.KeyExists(path, key, exists, cost).ok());
    BOOST_CHECK(exists);

    // Delete it.
    BOOST_CHECK(db.Delete(path, key, cost).ok());

    // Verify it no longer exists.
    BOOST_REQUIRE(db.KeyExists(path, key, exists, cost).ok());
    BOOST_CHECK(!exists);
}

BOOST_AUTO_TEST_CASE(test_delete_if_empty_tree)
{
    grovedb::test::TempDir tmp{"grovedb_test_delete_if_empty"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path path{};
    grovedb::Bytes tree_key{'t'};

    // Insert an empty subtree.
    grovedb::Element tree;
    BOOST_REQUIRE(grovedb::Element::EmptyTree(tree).ok());
    BOOST_REQUIRE(db.Put(path, tree_key, tree, cost).ok());

    // DeleteIfEmpty should succeed on an empty tree.
    bool deleted{false};
    BOOST_REQUIRE(db.DeleteIfEmpty(path, tree_key, deleted, cost).ok());
    BOOST_CHECK(deleted);

    // Verify it's gone.
    bool exists{true};
    BOOST_REQUIRE(db.KeyExists(path, tree_key, exists, cost).ok());
    BOOST_CHECK(!exists);
}

BOOST_AUTO_TEST_CASE(test_delete_if_empty_nonempty_tree)
{
    grovedb::test::TempDir tmp{"grovedb_test_delete_if_empty_nonempty"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path path{};
    grovedb::Bytes tree_key{'t'};

    // Insert a subtree and put an item inside it.
    grovedb::Element tree;
    BOOST_REQUIRE(grovedb::Element::EmptyTree(tree).ok());
    BOOST_REQUIRE(db.Put(path, tree_key, tree, cost).ok());

    grovedb::Element item;
    grovedb::Bytes value{'v'};
    BOOST_REQUIRE(grovedb::Element::Item(value, item).ok());
    grovedb::Path subtree_path{tree_key};
    grovedb::Bytes item_key{'i'};
    BOOST_REQUIRE(db.Put(subtree_path, item_key, item, cost).ok());

    // DeleteIfEmpty should not delete a non-empty tree.
    bool deleted{true};
    BOOST_REQUIRE(db.DeleteIfEmpty(path, tree_key, deleted, cost).ok());
    BOOST_CHECK(!deleted);

    // Tree should still exist.
    bool exists{false};
    BOOST_REQUIRE(db.KeyExists(path, tree_key, exists, cost).ok());
    BOOST_CHECK(exists);
}

BOOST_AUTO_TEST_CASE(test_clear_subtree)
{
    grovedb::test::TempDir tmp{"grovedb_test_clear_subtree"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path path{};
    grovedb::Bytes tree_key{'t'};

    // Create a subtree with items.
    grovedb::Element tree;
    BOOST_REQUIRE(grovedb::Element::EmptyTree(tree).ok());
    BOOST_REQUIRE(db.Put(path, tree_key, tree, cost).ok());

    grovedb::Path subtree_path{tree_key};
    grovedb::Element item;
    grovedb::Bytes value{'v'};
    BOOST_REQUIRE(grovedb::Element::Item(value, item).ok());
    BOOST_REQUIRE(db.Put(subtree_path, {'a'}, item, cost).ok());
    BOOST_REQUIRE(db.Put(subtree_path, {'b'}, item, cost).ok());

    // Clear the subtree.
    bool cleared{false};
    BOOST_REQUIRE(db.Clear(subtree_path, cleared).ok());
    BOOST_CHECK(cleared);

    // Items should be gone.
    bool exists{true};
    BOOST_REQUIRE(db.KeyExists(subtree_path, {'a'}, exists, cost).ok());
    BOOST_CHECK(!exists);
    BOOST_REQUIRE(db.KeyExists(subtree_path, {'b'}, exists, cost).ok());
    BOOST_CHECK(!exists);

    // The subtree itself should still exist.
    bool tree_exists{false};
    BOOST_REQUIRE(db.SubtreeExists(subtree_path, tree_exists, cost).ok());
    BOOST_CHECK(tree_exists);
}

BOOST_AUTO_TEST_CASE(test_delete_with_transaction)
{
    grovedb::test::TempDir tmp{"grovedb_test_delete_with_tx"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path path{};
    grovedb::Bytes key{'k'};

    // Insert an item.
    grovedb::Element item;
    grovedb::Bytes value{'v'};
    BOOST_REQUIRE(grovedb::Element::Item(value, item).ok());
    BOOST_REQUIRE(db.Put(path, key, item, cost).ok());

    // Delete within a transaction, then commit.
    grovedb::Transaction txn;
    BOOST_REQUIRE(db.BeginTransaction(txn).ok());
    BOOST_REQUIRE(db.Delete(path, key, txn, cost).ok());
    BOOST_REQUIRE(db.Commit(txn, cost).ok());

    // Should be gone.
    bool exists{true};
    BOOST_REQUIRE(db.KeyExists(path, key, exists, cost).ok());
    BOOST_CHECK(!exists);
}

BOOST_AUTO_TEST_CASE(test_prune_empty_ancestors)
{
    grovedb::test::TempDir tmp{"grovedb_test_prune_ancestors"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Create: root -> "a" (non-empty, has item "x") -> "b" -> "c" (empty).
    // Pruning "c" from ["a","b"] should delete "c" and then "b" (now empty),
    // but stop at "a" because "a" still contains item "x".
    grovedb::Element tree;
    BOOST_REQUIRE(grovedb::Element::EmptyTree(tree).ok());
    grovedb::Bytes key_a{'a'};
    grovedb::Bytes key_b{'b'};
    grovedb::Bytes key_c{'c'};
    BOOST_REQUIRE(db.Put(root, key_a, tree, cost).ok());

    // Add an item to "a" so it's not empty after "b" is removed.
    grovedb::Element item;
    grovedb::Bytes value{'v'};
    BOOST_REQUIRE(grovedb::Element::Item(value, item).ok());
    BOOST_REQUIRE(db.Put(grovedb::Path{key_a}, {'x'}, item, cost).ok());

    BOOST_REQUIRE(db.Put(grovedb::Path{key_a}, key_b, tree, cost).ok());
    BOOST_REQUIRE(db.Put(grovedb::Path{key_a, key_b}, key_c, tree, cost).ok());

    // Prune "c" from ["a","b"]: "c" is empty, "b" becomes empty after
    // "c" is removed, but "a" is not empty (has "x"), so pruning stops.
    uint32_t removed{0};
    auto status = db.PruneEmptyAncestors(grovedb::Path{key_a, key_b}, key_c, removed, cost);
    BOOST_CHECK_MESSAGE(status.ok(), status.message());
    BOOST_CHECK(removed > 0);

    // "b" should be gone but "a" should still exist.
    bool exists{true};
    BOOST_REQUIRE(db.KeyExists(grovedb::Path{key_a}, key_b, exists, cost).ok());
    BOOST_CHECK(!exists);

    BOOST_REQUIRE(db.KeyExists(root, key_a, exists, cost).ok());
    BOOST_CHECK(exists);
}

BOOST_AUTO_TEST_SUITE_END()

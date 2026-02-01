// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <grovedb/db.h>

#include <test/util/tempdir.h>

BOOST_AUTO_TEST_SUITE(insert_tests)

BOOST_AUTO_TEST_CASE(test_put_item)
{
    grovedb::test::TempDir tmp{"grovedb_test_put_item"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::Element item;
    grovedb::Bytes value{'h', 'e', 'l', 'l', 'o'};
    BOOST_REQUIRE(grovedb::Element::Item(value, item).ok());
    BOOST_CHECK(!item.empty());

    grovedb::OperationCost cost{};
    grovedb::Path path{};
    grovedb::Bytes key{'k'};

    BOOST_CHECK(db.Put(path, key, item, cost).ok());

    // Round-trip: get the element back and verify the serialized form matches.
    grovedb::Element fetched;
    BOOST_REQUIRE(db.GetDirect(path, key, fetched, cost).ok());
    BOOST_CHECK(!fetched.empty());
    BOOST_CHECK(fetched.data() == item.data());
}

BOOST_AUTO_TEST_CASE(test_put_empty_tree)
{
    grovedb::test::TempDir tmp{"grovedb_test_put_empty_tree"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::Element tree;
    BOOST_REQUIRE(grovedb::Element::EmptyTree(tree).ok());
    BOOST_CHECK(!tree.empty());

    grovedb::OperationCost cost{};
    grovedb::Path path{};
    grovedb::Bytes key{'t', 'r', 'e', 'e'};

    BOOST_CHECK(db.Put(path, key, tree, cost).ok());

    // The subtree path should now exist.
    bool exists{false};
    grovedb::Path subtree_path{key};
    BOOST_REQUIRE(db.SubtreeExists(subtree_path, exists, cost).ok());
    BOOST_CHECK(exists);
}

BOOST_AUTO_TEST_CASE(test_put_item_in_subtree)
{
    grovedb::test::TempDir tmp{"grovedb_test_put_item_in_subtree"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Bytes subtree_key{'s'};

    // Create a subtree at the root.
    grovedb::Element tree;
    BOOST_REQUIRE(grovedb::Element::EmptyTree(tree).ok());
    BOOST_REQUIRE(db.Put(grovedb::Path{}, subtree_key, tree, cost).ok());

    // Insert an item inside the subtree.
    grovedb::Element item;
    grovedb::Bytes value{'v', 'a', 'l'};
    BOOST_REQUIRE(grovedb::Element::Item(value, item).ok());

    grovedb::Path subtree_path{subtree_key};
    grovedb::Bytes item_key{'i'};
    BOOST_REQUIRE(db.Put(subtree_path, item_key, item, cost).ok());

    // Read back and verify round-trip.
    grovedb::Element fetched;
    BOOST_REQUIRE(db.GetDirect(subtree_path, item_key, fetched, cost).ok());
    BOOST_CHECK(fetched.data() == item.data());
}

BOOST_AUTO_TEST_CASE(test_put_if_absent)
{
    grovedb::test::TempDir tmp{"grovedb_test_put_if_absent"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::Element item;
    grovedb::Bytes value{'a'};
    BOOST_REQUIRE(grovedb::Element::Item(value, item).ok());

    grovedb::OperationCost cost{};
    grovedb::Path path{};
    grovedb::Bytes key{'k'};

    // First insert should succeed.
    bool inserted{false};
    BOOST_REQUIRE(db.PutIfAbsent(path, key, item, inserted, cost).ok());
    BOOST_CHECK(inserted);

    // Second insert with same key should not insert.
    inserted = true;
    BOOST_REQUIRE(db.PutIfAbsent(path, key, item, inserted, cost).ok());
    BOOST_CHECK(!inserted);
}

BOOST_AUTO_TEST_CASE(test_put_if_absent_and_get)
{
    grovedb::test::TempDir tmp{"grovedb_test_put_if_absent_get"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::Element item1;
    grovedb::Bytes value1{'a'};
    BOOST_REQUIRE(grovedb::Element::Item(value1, item1).ok());

    grovedb::OperationCost cost{};
    grovedb::Path path{};
    grovedb::Bytes key{'k'};

    // First insert: no existing element.
    std::optional<grovedb::Element> existing;
    BOOST_REQUIRE(db.PutIfAbsentAndGet(path, key, item1, existing, cost).ok());
    BOOST_CHECK(!existing.has_value());

    // Second insert with different value: should return the existing element.
    grovedb::Element item2;
    grovedb::Bytes value2{'b'};
    BOOST_REQUIRE(grovedb::Element::Item(value2, item2).ok());

    BOOST_REQUIRE(db.PutIfAbsentAndGet(path, key, item2, existing, cost).ok());
    BOOST_CHECK(existing.has_value());
    BOOST_CHECK(existing->data() == item1.data());
}

BOOST_AUTO_TEST_CASE(test_put_if_changed)
{
    grovedb::test::TempDir tmp{"grovedb_test_put_if_changed"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::Element item1;
    grovedb::Bytes value1{'v', '1'};
    BOOST_REQUIRE(grovedb::Element::Item(value1, item1).ok());

    grovedb::OperationCost cost{};
    grovedb::Path path{};
    grovedb::Bytes key{'k'};

    // First insert: key absent, changed=true, no previous element.
    bool changed{false};
    std::optional<grovedb::Element> previous;
    BOOST_REQUIRE(db.PutIfChanged(path, key, item1, changed, previous, cost).ok());
    BOOST_CHECK(changed);
    BOOST_CHECK(!previous.has_value());

    // Same value: not changed.
    changed = true;
    BOOST_REQUIRE(db.PutIfChanged(path, key, item1, changed, previous, cost).ok());
    BOOST_CHECK(!changed);

    // Different value: changed=true, previous holds old element.
    grovedb::Element item2;
    grovedb::Bytes value2{'v', '2'};
    BOOST_REQUIRE(grovedb::Element::Item(value2, item2).ok());

    BOOST_REQUIRE(db.PutIfChanged(path, key, item2, changed, previous, cost).ok());
    BOOST_CHECK(changed);
    BOOST_CHECK(previous.has_value());
    BOOST_CHECK(previous->data() == item1.data());
}

BOOST_AUTO_TEST_CASE(test_put_with_transaction)
{
    grovedb::test::TempDir tmp{"grovedb_test_put_with_tx"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::Element item;
    grovedb::Bytes value{'t', 'x'};
    BOOST_REQUIRE(grovedb::Element::Item(value, item).ok());

    grovedb::OperationCost cost{};
    grovedb::Path path{};
    grovedb::Bytes key{'k'};

    // Insert within a transaction.
    grovedb::Transaction txn;
    BOOST_REQUIRE(db.BeginTransaction(txn).ok());
    BOOST_REQUIRE(db.Put(path, key, item, txn, cost).ok());
    BOOST_REQUIRE(db.Commit(txn, cost).ok());

    // Committed data should be visible without a transaction.
    grovedb::Element fetched;
    BOOST_REQUIRE(db.GetDirect(path, key, fetched, cost).ok());
    BOOST_CHECK(fetched.data() == item.data());
}

BOOST_AUTO_TEST_CASE(test_put_if_absent_with_transaction)
{
    grovedb::test::TempDir tmp{"grovedb_test_put_absent_tx"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::Element item;
    grovedb::Bytes value{'a'};
    BOOST_REQUIRE(grovedb::Element::Item(value, item).ok());

    grovedb::OperationCost cost{};
    grovedb::Path path{};
    grovedb::Bytes key{'k'};

    // Insert in transaction.
    grovedb::Transaction txn;
    BOOST_REQUIRE(db.BeginTransaction(txn).ok());

    bool inserted{false};
    BOOST_REQUIRE(db.PutIfAbsent(path, key, item, txn, inserted, cost).ok());
    BOOST_CHECK(inserted);

    // Second attempt in same transaction should not insert.
    inserted = true;
    BOOST_REQUIRE(db.PutIfAbsent(path, key, item, txn, inserted, cost).ok());
    BOOST_CHECK(!inserted);

    BOOST_REQUIRE(db.Commit(txn, cost).ok());
}

BOOST_AUTO_TEST_SUITE_END()

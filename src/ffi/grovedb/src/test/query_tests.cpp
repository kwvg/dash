// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <grovedb/db.h>

#include <test/util/tempdir.h>

BOOST_AUTO_TEST_SUITE(query_tests)

BOOST_AUTO_TEST_CASE(test_query_single_key)
{
    grovedb::test::TempDir tmp{"grovedb_test_query_single_key"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Insert an item at root with key "k".
    grovedb::Element item;
    grovedb::Bytes value{'v', 'a', 'l'};
    BOOST_REQUIRE(grovedb::Element::Item(value, item).ok());
    BOOST_REQUIRE(db.Put(root, {'k'}, item, cost).ok());

    // Query for exactly that key.
    grovedb::PathQuery query;
    BOOST_REQUIRE(grovedb::PathQuery::New(
        root,
        {grovedb::QueryItem::Key({'k'})},
        0, 0,
        query).ok());

    std::vector<grovedb::Bytes> values;
    uint16_t skipped{0};
    auto status = db.QueryValues(query, values, skipped, cost);
    BOOST_CHECK_MESSAGE(status.ok(), status.message());
    BOOST_CHECK_EQUAL(values.size(), 1);
    BOOST_CHECK(values[0] == value);
    BOOST_CHECK_EQUAL(skipped, 0);
}

BOOST_AUTO_TEST_CASE(test_query_range_full)
{
    grovedb::test::TempDir tmp{"grovedb_test_query_range_full"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Insert three items.
    grovedb::Element item;
    BOOST_REQUIRE(grovedb::Element::Item({'1'}, item).ok());
    BOOST_REQUIRE(db.Put(root, {'a'}, item, cost).ok());
    BOOST_REQUIRE(grovedb::Element::Item({'2'}, item).ok());
    BOOST_REQUIRE(db.Put(root, {'b'}, item, cost).ok());
    BOOST_REQUIRE(grovedb::Element::Item({'3'}, item).ok());
    BOOST_REQUIRE(db.Put(root, {'c'}, item, cost).ok());

    // Query for all keys.
    grovedb::PathQuery query;
    BOOST_REQUIRE(grovedb::PathQuery::New(
        root,
        {grovedb::QueryItem::RangeFull()},
        0, 0,
        query).ok());

    std::vector<grovedb::Bytes> values;
    uint16_t skipped{0};
    BOOST_REQUIRE(db.QueryValues(query, values, skipped, cost).ok());
    BOOST_CHECK_EQUAL(values.size(), 3);
}

BOOST_AUTO_TEST_CASE(test_query_with_limit)
{
    grovedb::test::TempDir tmp{"grovedb_test_query_with_limit"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Insert five items.
    grovedb::Element item;
    for (uint8_t i = 0; i < 5; ++i) {
        BOOST_REQUIRE(grovedb::Element::Item({i}, item).ok());
        BOOST_REQUIRE(db.Put(root, {static_cast<uint8_t>('a' + i)}, item, cost).ok());
    }

    // Query with limit = 2.
    grovedb::PathQuery query;
    BOOST_REQUIRE(grovedb::PathQuery::New(
        root,
        {grovedb::QueryItem::RangeFull()},
        2, 0,
        query).ok());

    std::vector<grovedb::Bytes> values;
    uint16_t skipped{0};
    BOOST_REQUIRE(db.QueryValues(query, values, skipped, cost).ok());
    BOOST_CHECK_EQUAL(values.size(), 2);
}

BOOST_AUTO_TEST_CASE(test_query_with_offset)
{
    grovedb::test::TempDir tmp{"grovedb_test_query_with_offset"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Insert three items: a=1, b=2, c=3.
    grovedb::Element item;
    BOOST_REQUIRE(grovedb::Element::Item({'1'}, item).ok());
    BOOST_REQUIRE(db.Put(root, {'a'}, item, cost).ok());
    BOOST_REQUIRE(grovedb::Element::Item({'2'}, item).ok());
    BOOST_REQUIRE(db.Put(root, {'b'}, item, cost).ok());
    BOOST_REQUIRE(grovedb::Element::Item({'3'}, item).ok());
    BOOST_REQUIRE(db.Put(root, {'c'}, item, cost).ok());

    // Query all with offset = 1: should skip 'a' and return 'b','c'.
    grovedb::PathQuery query;
    BOOST_REQUIRE(grovedb::PathQuery::New(
        root,
        {grovedb::QueryItem::RangeFull()},
        0, 1,
        query).ok());

    std::vector<grovedb::Bytes> values;
    uint16_t skipped{0};
    BOOST_REQUIRE(db.QueryValues(query, values, skipped, cost).ok());
    BOOST_CHECK_EQUAL(values.size(), 2);
    BOOST_CHECK_EQUAL(skipped, 1);
    BOOST_CHECK(values[0] == grovedb::Bytes{'2'});
    BOOST_CHECK(values[1] == grovedb::Bytes{'3'});
}

BOOST_AUTO_TEST_CASE(test_query_range_inclusive)
{
    grovedb::test::TempDir tmp{"grovedb_test_query_range_inclusive"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Insert items with keys a, b, c, d.
    grovedb::Element item;
    for (uint8_t k = 'a'; k <= 'd'; ++k) {
        BOOST_REQUIRE(grovedb::Element::Item({k}, item).ok());
        BOOST_REQUIRE(db.Put(root, {k}, item, cost).ok());
    }

    // RangeInclusive [b, c] should return 2 items.
    grovedb::PathQuery query;
    BOOST_REQUIRE(grovedb::PathQuery::New(
        root,
        {grovedb::QueryItem::RangeInclusive({'b'}, {'c'})},
        0, 0,
        query).ok());

    std::vector<grovedb::Bytes> values;
    uint16_t skipped{0};
    BOOST_REQUIRE(db.QueryValues(query, values, skipped, cost).ok());
    BOOST_CHECK_EQUAL(values.size(), 2);
    BOOST_CHECK(values[0] == grovedb::Bytes{'b'});
    BOOST_CHECK(values[1] == grovedb::Bytes{'c'});
}

BOOST_AUTO_TEST_CASE(test_query_with_transaction)
{
    grovedb::test::TempDir tmp{"grovedb_test_query_with_tx"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Insert an item within a transaction.
    grovedb::Transaction txn;
    BOOST_REQUIRE(db.BeginTransaction(txn).ok());

    grovedb::Element item;
    BOOST_REQUIRE(grovedb::Element::Item({'x'}, item).ok());
    BOOST_REQUIRE(db.Put(root, {'k'}, item, txn, cost).ok());

    // Query within the same transaction should find the item.
    grovedb::PathQuery query;
    BOOST_REQUIRE(grovedb::PathQuery::New(
        root,
        {grovedb::QueryItem::Key({'k'})},
        0, 0,
        query).ok());

    std::vector<grovedb::Bytes> values;
    uint16_t skipped{0};
    BOOST_REQUIRE(db.QueryValues(query, txn, values, skipped, cost).ok());
    BOOST_CHECK_EQUAL(values.size(), 1);
    BOOST_CHECK(values[0] == grovedb::Bytes{'x'});

    BOOST_REQUIRE(db.Commit(txn, cost).ok());
}

BOOST_AUTO_TEST_CASE(test_query_subtree)
{
    grovedb::test::TempDir tmp{"grovedb_test_query_subtree"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Create a subtree "t" and put items inside it.
    grovedb::Element tree;
    BOOST_REQUIRE(grovedb::Element::EmptyTree(tree).ok());
    BOOST_REQUIRE(db.Put(root, {'t'}, tree, cost).ok());

    grovedb::Element item;
    grovedb::Path subtree{{'t'}};
    BOOST_REQUIRE(grovedb::Element::Item({'1'}, item).ok());
    BOOST_REQUIRE(db.Put(subtree, {'a'}, item, cost).ok());
    BOOST_REQUIRE(grovedb::Element::Item({'2'}, item).ok());
    BOOST_REQUIRE(db.Put(subtree, {'b'}, item, cost).ok());

    // Query all items in the subtree.
    grovedb::PathQuery query;
    BOOST_REQUIRE(grovedb::PathQuery::New(
        subtree,
        {grovedb::QueryItem::RangeFull()},
        0, 0,
        query).ok());

    std::vector<grovedb::Bytes> values;
    uint16_t skipped{0};
    BOOST_REQUIRE(db.QueryValues(query, values, skipped, cost).ok());
    BOOST_CHECK_EQUAL(values.size(), 2);
    BOOST_CHECK(values[0] == grovedb::Bytes{'1'});
    BOOST_CHECK(values[1] == grovedb::Bytes{'2'});
}

BOOST_AUTO_TEST_CASE(test_query_with_subquery)
{
    grovedb::test::TempDir tmp{"grovedb_test_query_with_subquery"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Create two subtrees "s1" and "s2" at root.
    grovedb::Element tree;
    BOOST_REQUIRE(grovedb::Element::EmptyTree(tree).ok());
    BOOST_REQUIRE(db.Put(root, {'s', '1'}, tree, cost).ok());
    BOOST_REQUIRE(grovedb::Element::EmptyTree(tree).ok());
    BOOST_REQUIRE(db.Put(root, {'s', '2'}, tree, cost).ok());

    // Put items in each subtree.
    grovedb::Element item;
    BOOST_REQUIRE(grovedb::Element::Item({'a'}, item).ok());
    BOOST_REQUIRE(db.Put(grovedb::Path{{'s', '1'}}, {'x'}, item, cost).ok());
    BOOST_REQUIRE(grovedb::Element::Item({'b'}, item).ok());
    BOOST_REQUIRE(db.Put(grovedb::Path{{'s', '2'}}, {'x'}, item, cost).ok());

    // Query across both subtrees using a subquery.
    // Outer: RangeFull at root (selects s1 and s2).
    // Subquery: Key "x" in each subtree.
    grovedb::PathQuery query;
    BOOST_REQUIRE(grovedb::PathQuery::NewWithSubquery(
        root,
        {grovedb::QueryItem::RangeFull()},
        0, 0,
        {},  // no subquery path navigation
        {grovedb::QueryItem::Key({'x'})},
        query).ok());

    std::vector<grovedb::Bytes> values;
    uint16_t skipped{0};
    BOOST_REQUIRE(db.QueryValues(query, values, skipped, cost).ok());
    BOOST_CHECK_EQUAL(values.size(), 2);
    BOOST_CHECK(values[0] == grovedb::Bytes{'a'});
    BOOST_CHECK(values[1] == grovedb::Bytes{'b'});
}

// ---------------------------------------------------------------------------
// QueryItemsOrSums — SumValue variant
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_query_items_or_sums_sum_value)
{
    grovedb::test::TempDir tmp{"grovedb_test_items_or_sums_sum"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Create a sum tree at root with key "s".
    grovedb::Element sum_tree;
    BOOST_REQUIRE(grovedb::Element::EmptySumTree(sum_tree).ok());
    BOOST_REQUIRE(db.Put(root, {'s'}, sum_tree, cost).ok());

    // Insert sum items into the sum tree.
    grovedb::Path sum_path{{'s'}};
    grovedb::Element si;
    BOOST_REQUIRE(grovedb::Element::SumItem(42, si).ok());
    BOOST_REQUIRE(db.Put(sum_path, {'a'}, si, cost).ok());
    BOOST_REQUIRE(grovedb::Element::SumItem(-7, si).ok());
    BOOST_REQUIRE(db.Put(sum_path, {'b'}, si, cost).ok());

    // Query all items in the sum tree.
    grovedb::PathQuery query;
    BOOST_REQUIRE(grovedb::PathQuery::New(
        sum_path,
        {grovedb::QueryItem::RangeFull()},
        0, 0,
        query).ok());

    std::vector<grovedb::QueryItemOrSum> results;
    uint16_t skipped{0};
    auto status = db.QueryItemsOrSums(query, results, skipped, cost);
    BOOST_CHECK_MESSAGE(status.ok(), status.message());
    BOOST_CHECK_EQUAL(results.size(), 2);
    // Sum items should come back as ItemData (serialized element bytes).
    // The kind depends on the GroveDB implementation for sum items.
    BOOST_CHECK_EQUAL(skipped, 0);
}

// ---------------------------------------------------------------------------
// QueryItemsOrSums — ItemData variant
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_query_items_or_sums_item_data)
{
    grovedb::test::TempDir tmp{"grovedb_test_items_or_sums_item"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Insert regular items.
    grovedb::Element item;
    BOOST_REQUIRE(grovedb::Element::Item({'v', '1'}, item).ok());
    BOOST_REQUIRE(db.Put(root, {'a'}, item, cost).ok());
    BOOST_REQUIRE(grovedb::Element::Item({'v', '2'}, item).ok());
    BOOST_REQUIRE(db.Put(root, {'b'}, item, cost).ok());

    grovedb::PathQuery query;
    BOOST_REQUIRE(grovedb::PathQuery::New(
        root,
        {grovedb::QueryItem::RangeFull()},
        0, 0,
        query).ok());

    std::vector<grovedb::QueryItemOrSum> results;
    uint16_t skipped{0};
    auto status = db.QueryItemsOrSums(query, results, skipped, cost);
    BOOST_CHECK_MESSAGE(status.ok(), status.message());
    BOOST_CHECK_EQUAL(results.size(), 2);
    for (const auto& r : results) {
        BOOST_CHECK(r.m_kind == grovedb::QueryItemOrSum::Kind::ItemData);
        BOOST_CHECK(!r.m_item_data.empty());
    }
}

// ---------------------------------------------------------------------------
// QuerySums
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_query_sums)
{
    grovedb::test::TempDir tmp{"grovedb_test_query_sums"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Create a sum tree.
    grovedb::Element sum_tree;
    BOOST_REQUIRE(grovedb::Element::EmptySumTree(sum_tree).ok());
    BOOST_REQUIRE(db.Put(root, {'s'}, sum_tree, cost).ok());

    // Insert sum items.
    grovedb::Path sum_path{{'s'}};
    grovedb::Element si;
    BOOST_REQUIRE(grovedb::Element::SumItem(100, si).ok());
    BOOST_REQUIRE(db.Put(sum_path, {'x'}, si, cost).ok());
    BOOST_REQUIRE(grovedb::Element::SumItem(200, si).ok());
    BOOST_REQUIRE(db.Put(sum_path, {'y'}, si, cost).ok());
    BOOST_REQUIRE(grovedb::Element::SumItem(-50, si).ok());
    BOOST_REQUIRE(db.Put(sum_path, {'z'}, si, cost).ok());

    grovedb::PathQuery query;
    BOOST_REQUIRE(grovedb::PathQuery::New(
        sum_path,
        {grovedb::QueryItem::RangeFull()},
        0, 0,
        query).ok());

    std::vector<int64_t> sums;
    uint16_t skipped{0};
    auto status = db.QuerySums(query, sums, skipped, cost);
    BOOST_CHECK_MESSAGE(status.ok(), status.message());
    BOOST_CHECK_EQUAL(sums.size(), 3);

    // Sum items should return their values in key order.
    BOOST_CHECK_EQUAL(sums[0], 100);
    BOOST_CHECK_EQUAL(sums[1], 200);
    BOOST_CHECK_EQUAL(sums[2], -50);
}

// ---------------------------------------------------------------------------
// QueryRaw — Element result type
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_query_raw_element)
{
    grovedb::test::TempDir tmp{"grovedb_test_query_raw_elem"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    grovedb::Element item;
    BOOST_REQUIRE(grovedb::Element::Item({'v'}, item).ok());
    BOOST_REQUIRE(db.Put(root, {'k'}, item, cost).ok());

    grovedb::PathQuery query;
    BOOST_REQUIRE(grovedb::PathQuery::New(
        root,
        {grovedb::QueryItem::Key({'k'})},
        0, 0,
        query).ok());

    std::vector<grovedb::QueryResultElement> elements;
    uint16_t skipped{0};
    auto status = db.QueryRaw(query, 0, elements, skipped, cost);
    BOOST_CHECK_MESSAGE(status.ok(), status.message());
    BOOST_CHECK_EQUAL(elements.size(), 1);
    BOOST_CHECK(elements[0].m_kind == grovedb::QueryResultElement::Kind::Element);
    BOOST_CHECK(!elements[0].m_element.data().empty());
}

// ---------------------------------------------------------------------------
// QueryRaw — KeyElementPair result type
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_query_raw_key_element_pair)
{
    grovedb::test::TempDir tmp{"grovedb_test_query_raw_kep"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    grovedb::Element item;
    BOOST_REQUIRE(grovedb::Element::Item({'v'}, item).ok());
    BOOST_REQUIRE(db.Put(root, {'k'}, item, cost).ok());

    grovedb::PathQuery query;
    BOOST_REQUIRE(grovedb::PathQuery::New(
        root,
        {grovedb::QueryItem::Key({'k'})},
        0, 0,
        query).ok());

    std::vector<grovedb::QueryResultElement> elements;
    uint16_t skipped{0};
    auto status = db.QueryRaw(query, 1, elements, skipped, cost);
    BOOST_CHECK_MESSAGE(status.ok(), status.message());
    BOOST_CHECK_EQUAL(elements.size(), 1);
    BOOST_CHECK(elements[0].m_kind == grovedb::QueryResultElement::Kind::KeyElementPair);
    BOOST_CHECK(elements[0].m_key == grovedb::Bytes{'k'});
    BOOST_CHECK(!elements[0].m_element.data().empty());
}

// ---------------------------------------------------------------------------
// QueryRaw — PathKeyElementTrio result type
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_query_raw_path_key_element_trio)
{
    grovedb::test::TempDir tmp{"grovedb_test_query_raw_pket"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    grovedb::Element item;
    BOOST_REQUIRE(grovedb::Element::Item({'v'}, item).ok());
    BOOST_REQUIRE(db.Put(root, {'k'}, item, cost).ok());

    grovedb::PathQuery query;
    BOOST_REQUIRE(grovedb::PathQuery::New(
        root,
        {grovedb::QueryItem::Key({'k'})},
        0, 0,
        query).ok());

    std::vector<grovedb::QueryResultElement> elements;
    uint16_t skipped{0};
    auto status = db.QueryRaw(query, 2, elements, skipped, cost);
    BOOST_CHECK_MESSAGE(status.ok(), status.message());
    BOOST_CHECK_EQUAL(elements.size(), 1);
    BOOST_CHECK(elements[0].m_kind == grovedb::QueryResultElement::Kind::PathKeyElementTrio);
    BOOST_CHECK(elements[0].m_key == grovedb::Bytes{'k'});
    BOOST_CHECK(elements[0].m_path.empty()); // root path is empty
    BOOST_CHECK(!elements[0].m_element.data().empty());
}

// ---------------------------------------------------------------------------
// QueryManyRaw
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_query_many_raw)
{
    grovedb::test::TempDir tmp{"grovedb_test_query_many_raw"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Insert items at root.
    grovedb::Element item;
    BOOST_REQUIRE(grovedb::Element::Item({'1'}, item).ok());
    BOOST_REQUIRE(db.Put(root, {'a'}, item, cost).ok());
    BOOST_REQUIRE(grovedb::Element::Item({'2'}, item).ok());
    BOOST_REQUIRE(db.Put(root, {'b'}, item, cost).ok());

    // Create two queries for individual keys.
    std::vector<grovedb::QueryItem> items_a{grovedb::QueryItem::Key({'a'})};
    std::vector<grovedb::QueryItem> items_b{grovedb::QueryItem::Key({'b'})};

    std::vector<grovedb::Db::RawQuerySpec> queries{
        {.path = root, .items = items_a, .limit = 0, .offset = 0},
        {.path = root, .items = items_b, .limit = 0, .offset = 0},
    };

    std::vector<grovedb::QueryResultElement> elements;
    auto status = db.QueryManyRaw(queries, 0, elements, cost);
    BOOST_CHECK_MESSAGE(status.ok(), status.message());
    BOOST_CHECK_EQUAL(elements.size(), 2);
}

// ---------------------------------------------------------------------------
// QueryKeysOptional — existing and missing keys
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_query_keys_optional)
{
    grovedb::test::TempDir tmp{"grovedb_test_query_keys_opt"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Insert one item.
    grovedb::Element item;
    BOOST_REQUIRE(grovedb::Element::Item({'v'}, item).ok());
    BOOST_REQUIRE(db.Put(root, {'a'}, item, cost).ok());

    // Query for existing key 'a' and non-existing key 'z'.
    // query_keys_optional requires a limit.
    grovedb::PathQuery query;
    BOOST_REQUIRE(grovedb::PathQuery::New(
        root,
        {grovedb::QueryItem::Key({'a'}), grovedb::QueryItem::Key({'z'})},
        100, 0,
        query).ok());

    std::vector<grovedb::PathKeyElement> results;
    auto status = db.QueryKeysOptional(query, results, cost);
    BOOST_CHECK_MESSAGE(status.ok(), status.message());
    BOOST_CHECK_EQUAL(results.size(), 2);

    // Key 'a' should have an element.
    BOOST_CHECK(results[0].m_key == grovedb::Bytes{'a'});
    BOOST_CHECK(results[0].m_element.has_value());

    // Key 'z' should be absent.
    BOOST_CHECK(results[1].m_key == grovedb::Bytes{'z'});
    BOOST_CHECK(!results[1].m_element.has_value());
}

// ---------------------------------------------------------------------------
// QueryRawKeysOptional
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_query_raw_keys_optional)
{
    grovedb::test::TempDir tmp{"grovedb_test_query_raw_keys_opt"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    grovedb::Element item;
    BOOST_REQUIRE(grovedb::Element::Item({'v'}, item).ok());
    BOOST_REQUIRE(db.Put(root, {'x'}, item, cost).ok());

    grovedb::PathQuery query;
    BOOST_REQUIRE(grovedb::PathQuery::New(
        root,
        {grovedb::QueryItem::Key({'x'}), grovedb::QueryItem::Key({'y'})},
        100, 0,
        query).ok());

    std::vector<grovedb::PathKeyElement> results;
    auto status = db.QueryRawKeysOptional(query, results, cost);
    BOOST_CHECK_MESSAGE(status.ok(), status.message());
    BOOST_CHECK_EQUAL(results.size(), 2);

    BOOST_CHECK(results[0].m_key == grovedb::Bytes{'x'});
    BOOST_CHECK(results[0].m_element.has_value());

    BOOST_CHECK(results[1].m_key == grovedb::Bytes{'y'});
    BOOST_CHECK(!results[1].m_element.has_value());
}

BOOST_AUTO_TEST_SUITE_END()

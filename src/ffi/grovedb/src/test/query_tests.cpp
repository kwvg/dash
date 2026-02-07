// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <test/util/tempdir.h>

#include <grovedb/db.h>
#include <grovedb/element.h>
#include <grovedb/query.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(query_tests)

BOOST_AUTO_TEST_CASE(query_single_key)
{
  grovedb::test::TempDir dir("q_single_key");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path root{};
  auto value = grovedb::Bytes::FromString("val");
  auto item = grovedb::Element::Item(value);
  BOOST_REQUIRE(item.has_value());
  BOOST_REQUIRE(db->Put(root, {'k'}, *item).has_value());

  auto result = grovedb::PathQuery::New(root, {grovedb::QueryItem::Key({'k'})})
                    .and_then([&](grovedb::PathQuery q) { return db->QueryValues(q); });
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK_EQUAL(result->value().m_data.size(), 1u);
  BOOST_CHECK(result->value().m_data[0] == value);
  BOOST_CHECK_EQUAL(result->value().m_skipped, 0u);
}

BOOST_AUTO_TEST_CASE(query_range_full)
{
  grovedb::test::TempDir dir("q_range_full");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path root{};
  for (uint8_t i = 0; i < 3; ++i) {
    auto elem = grovedb::Element::Item({static_cast<uint8_t>('1' + i)});
    BOOST_REQUIRE(elem.has_value());
    BOOST_REQUIRE(db->Put(root, {static_cast<uint8_t>('a' + i)}, *elem).has_value());
  }

  auto result = grovedb::PathQuery::New(root, {grovedb::QueryItem::RangeFull()})
                    .and_then([&](grovedb::PathQuery q) { return db->QueryValues(q); });
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK_EQUAL(result->value().m_data.size(), 3u);
}

BOOST_AUTO_TEST_CASE(query_with_limit)
{
  grovedb::test::TempDir dir("q_limit");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path root{};
  for (uint8_t i = 0; i < 5; ++i) {
    auto elem = grovedb::Element::Item({i});
    BOOST_REQUIRE(elem.has_value());
    BOOST_REQUIRE(db->Put(root, {static_cast<uint8_t>('a' + i)}, *elem).has_value());
  }

  auto result = grovedb::PathQuery::New(root, {grovedb::QueryItem::RangeFull()}, /*limit=*/2)
                    .and_then([&](grovedb::PathQuery q) { return db->QueryValues(q); });
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK_EQUAL(result->value().m_data.size(), 2u);
}

BOOST_AUTO_TEST_CASE(query_with_offset)
{
  grovedb::test::TempDir dir("q_offset");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path root{};
  auto e1 = grovedb::Element::Item({'1'});
  auto e2 = grovedb::Element::Item({'2'});
  auto e3 = grovedb::Element::Item({'3'});
  BOOST_REQUIRE(e1.has_value());
  BOOST_REQUIRE(e2.has_value());
  BOOST_REQUIRE(e3.has_value());
  BOOST_REQUIRE(db->Put(root, {'a'}, *e1).has_value());
  BOOST_REQUIRE(db->Put(root, {'b'}, *e2).has_value());
  BOOST_REQUIRE(db->Put(root, {'c'}, *e3).has_value());

  auto result =
      grovedb::PathQuery::New(root, {grovedb::QueryItem::RangeFull()}, /*limit=*/0, /*offset=*/1)
          .and_then([&](grovedb::PathQuery q) { return db->QueryValues(q); });
  BOOST_REQUIRE(result.has_value());
  auto& data = result->value();
  BOOST_CHECK_EQUAL(data.m_data.size(), 2u);
  BOOST_CHECK_EQUAL(data.m_skipped, 1u);
  BOOST_CHECK(data.m_data[0] == grovedb::Bytes{'2'});
  BOOST_CHECK(data.m_data[1] == grovedb::Bytes{'3'});
}

BOOST_AUTO_TEST_CASE(query_range_inclusive)
{
  grovedb::test::TempDir dir("q_range_incl");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path root{};
  for (uint8_t k = 'a'; k <= 'd'; ++k) {
    auto elem = grovedb::Element::Item({k});
    BOOST_REQUIRE(elem.has_value());
    BOOST_REQUIRE(db->Put(root, {k}, *elem).has_value());
  }

  auto result = grovedb::PathQuery::New(root, {grovedb::QueryItem::RangeInclusive({'b'}, {'c'})})
                    .and_then([&](grovedb::PathQuery q) { return db->QueryValues(q); });
  BOOST_REQUIRE(result.has_value());
  auto& data = result->value();
  BOOST_CHECK_EQUAL(data.m_data.size(), 2u);
  BOOST_CHECK(data.m_data[0] == grovedb::Bytes{'b'});
  BOOST_CHECK(data.m_data[1] == grovedb::Bytes{'c'});
}

BOOST_AUTO_TEST_CASE(query_with_transaction)
{
  grovedb::test::TempDir dir("q_with_tx");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path root{};
  auto tx = db->BeginTransaction();
  BOOST_REQUIRE(tx.has_value());

  auto item = grovedb::Element::Item({'x'});
  BOOST_REQUIRE(item.has_value());
  BOOST_REQUIRE(db->Put(root, {'k'}, *item, *tx).has_value());

  auto result = grovedb::PathQuery::New(root, {grovedb::QueryItem::Key({'k'})})
                    .and_then([&](grovedb::PathQuery q) { return db->QueryValues(q, *tx); });
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK_EQUAL(result->value().m_data.size(), 1u);
  BOOST_CHECK(result->value().m_data[0] == grovedb::Bytes{'x'});

  BOOST_REQUIRE(db->Commit(*tx).has_value());
}

BOOST_AUTO_TEST_CASE(query_subtree)
{
  grovedb::test::TempDir dir("q_subtree");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path root{};
  auto tree = grovedb::Element::EmptyTree();
  BOOST_REQUIRE(tree.has_value());
  BOOST_REQUIRE(db->Put(root, {'t'}, *tree).has_value());

  grovedb::Path subtree{{'t'}};
  auto i1 = grovedb::Element::Item({'1'});
  auto i2 = grovedb::Element::Item({'2'});
  BOOST_REQUIRE(i1.has_value());
  BOOST_REQUIRE(i2.has_value());
  BOOST_REQUIRE(db->Put(subtree, {'a'}, *i1).has_value());
  BOOST_REQUIRE(db->Put(subtree, {'b'}, *i2).has_value());

  auto result = grovedb::PathQuery::New(subtree, {grovedb::QueryItem::RangeFull()})
                    .and_then([&](grovedb::PathQuery q) { return db->QueryValues(q); });
  BOOST_REQUIRE(result.has_value());
  auto& data = result->value();
  BOOST_CHECK_EQUAL(data.m_data.size(), 2u);
  BOOST_CHECK(data.m_data[0] == grovedb::Bytes{'1'});
  BOOST_CHECK(data.m_data[1] == grovedb::Bytes{'2'});
}

BOOST_AUTO_TEST_CASE(query_with_subquery)
{
  grovedb::test::TempDir dir("q_subquery");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path root{};
  auto tree = grovedb::Element::EmptyTree();
  BOOST_REQUIRE(tree.has_value());
  BOOST_REQUIRE(db->Put(root, grovedb::Bytes::FromString("s1"), *tree).has_value());
  tree = grovedb::Element::EmptyTree();
  BOOST_REQUIRE(tree.has_value());
  BOOST_REQUIRE(db->Put(root, grovedb::Bytes::FromString("s2"), *tree).has_value());

  auto ia = grovedb::Element::Item({'a'});
  auto ib = grovedb::Element::Item({'b'});
  BOOST_REQUIRE(ia.has_value());
  BOOST_REQUIRE(ib.has_value());
  BOOST_REQUIRE(db->Put(grovedb::Path{grovedb::Bytes::FromString("s1")}, {'x'}, *ia).has_value());
  BOOST_REQUIRE(db->Put(grovedb::Path{grovedb::Bytes::FromString("s2")}, {'x'}, *ib).has_value());

  auto result = grovedb::PathQuery::NewWithSubquery(
                    root,
                    {grovedb::QueryItem::RangeFull()},
                    /*limit=*/0,
                    /*offset=*/0,
                    {},
                    {grovedb::QueryItem::Key({'x'})}
  )
                    .and_then([&](grovedb::PathQuery q) { return db->QueryValues(q); });
  BOOST_REQUIRE(result.has_value());
  auto& data = result->value();
  BOOST_CHECK_EQUAL(data.m_data.size(), 2u);
  BOOST_CHECK(data.m_data[0] == grovedb::Bytes{'a'});
  BOOST_CHECK(data.m_data[1] == grovedb::Bytes{'b'});
}

BOOST_AUTO_TEST_CASE(query_items_or_sums_sum_value)
{
  grovedb::test::TempDir dir("q_ios_sum");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path root{};
  auto sum_tree = grovedb::Element::EmptySumTree();
  BOOST_REQUIRE(sum_tree.has_value());
  BOOST_REQUIRE(db->Put(root, {'s'}, *sum_tree).has_value());

  grovedb::Path sum_path{{'s'}};
  auto s1 = grovedb::Element::SumItem(42);
  auto s2 = grovedb::Element::SumItem(-7);
  BOOST_REQUIRE(s1.has_value());
  BOOST_REQUIRE(s2.has_value());
  BOOST_REQUIRE(db->Put(sum_path, {'a'}, *s1).has_value());
  BOOST_REQUIRE(db->Put(sum_path, {'b'}, *s2).has_value());

  auto result = grovedb::PathQuery::New(sum_path, {grovedb::QueryItem::RangeFull()})
                    .and_then([&](grovedb::PathQuery q) { return db->QueryItemsOrSums(q); });
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK_EQUAL(result->value().m_data.size(), 2u);
  BOOST_CHECK_EQUAL(result->value().m_skipped, 0u);
}

BOOST_AUTO_TEST_CASE(query_items_or_sums_item_data)
{
  grovedb::test::TempDir dir("q_ios_item");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path root{};
  auto e1 = grovedb::Element::Item(grovedb::Bytes::FromString("v1"));
  auto e2 = grovedb::Element::Item(grovedb::Bytes::FromString("v2"));
  BOOST_REQUIRE(e1.has_value());
  BOOST_REQUIRE(e2.has_value());
  BOOST_REQUIRE(db->Put(root, {'a'}, *e1).has_value());
  BOOST_REQUIRE(db->Put(root, {'b'}, *e2).has_value());

  auto result = grovedb::PathQuery::New(root, {grovedb::QueryItem::RangeFull()})
                    .and_then([&](grovedb::PathQuery q) { return db->QueryItemsOrSums(q); });
  BOOST_REQUIRE(result.has_value());
  auto& items = result->value().m_data;
  BOOST_CHECK_EQUAL(items.size(), 2u);
  for (const auto& item : items) {
    BOOST_CHECK(item.m_kind == grovedb::QueryItemOrSumKind::ItemData);
    BOOST_CHECK(!item.m_item_data.empty());
  }
}

BOOST_AUTO_TEST_CASE(query_sums)
{
  grovedb::test::TempDir dir("q_sums");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path root{};
  auto sum_tree = grovedb::Element::EmptySumTree();
  BOOST_REQUIRE(sum_tree.has_value());
  BOOST_REQUIRE(db->Put(root, {'s'}, *sum_tree).has_value());

  grovedb::Path sum_path{{'s'}};
  auto s1 = grovedb::Element::SumItem(100);
  auto s2 = grovedb::Element::SumItem(200);
  auto s3 = grovedb::Element::SumItem(-50);
  BOOST_REQUIRE(s1.has_value());
  BOOST_REQUIRE(s2.has_value());
  BOOST_REQUIRE(s3.has_value());
  BOOST_REQUIRE(db->Put(sum_path, {'x'}, *s1).has_value());
  BOOST_REQUIRE(db->Put(sum_path, {'y'}, *s2).has_value());
  BOOST_REQUIRE(db->Put(sum_path, {'z'}, *s3).has_value());

  auto result = grovedb::PathQuery::New(sum_path, {grovedb::QueryItem::RangeFull()})
                    .and_then([&](grovedb::PathQuery q) { return db->QuerySums(q); });
  BOOST_REQUIRE(result.has_value());
  auto& sums = result->value().m_data;
  BOOST_CHECK_EQUAL(sums.size(), 3u);
  BOOST_CHECK_EQUAL(sums[0], 100);
  BOOST_CHECK_EQUAL(sums[1], 200);
  BOOST_CHECK_EQUAL(sums[2], -50);
}

BOOST_AUTO_TEST_CASE(query_raw_element)
{
  grovedb::test::TempDir dir("q_raw_elem");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path root{};
  auto item = grovedb::Element::Item({'v'});
  BOOST_REQUIRE(item.has_value());
  BOOST_REQUIRE(db->Put(root, {'k'}, *item).has_value());

  auto result =
      grovedb::PathQuery::New(root, {grovedb::QueryItem::Key({'k'})})
          .and_then([&](grovedb::PathQuery q) { return db->QueryRaw(q, /*result_type=*/0); });
  BOOST_REQUIRE(result.has_value());
  auto& elems = result->value().m_data;
  BOOST_CHECK_EQUAL(elems.size(), 1u);
  BOOST_CHECK(elems[0].m_kind == grovedb::QueryResultKind::Element);
  BOOST_CHECK(!elems[0].m_element.empty());
}

BOOST_AUTO_TEST_CASE(query_raw_key_element_pair)
{
  grovedb::test::TempDir dir("q_raw_kep");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path root{};
  auto item = grovedb::Element::Item({'v'});
  BOOST_REQUIRE(item.has_value());
  BOOST_REQUIRE(db->Put(root, {'k'}, *item).has_value());

  auto result =
      grovedb::PathQuery::New(root, {grovedb::QueryItem::Key({'k'})})
          .and_then([&](grovedb::PathQuery q) { return db->QueryRaw(q, /*result_type=*/1); });
  BOOST_REQUIRE(result.has_value());
  auto& elems = result->value().m_data;
  BOOST_CHECK_EQUAL(elems.size(), 1u);
  BOOST_CHECK(elems[0].m_kind == grovedb::QueryResultKind::KeyElementPair);
  BOOST_CHECK(elems[0].m_key == grovedb::Bytes{'k'});
  BOOST_CHECK(!elems[0].m_element.empty());
}

BOOST_AUTO_TEST_CASE(query_raw_path_key_element_trio)
{
  grovedb::test::TempDir dir("q_raw_pket");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path root{};
  auto item = grovedb::Element::Item({'v'});
  BOOST_REQUIRE(item.has_value());
  BOOST_REQUIRE(db->Put(root, {'k'}, *item).has_value());

  auto result =
      grovedb::PathQuery::New(root, {grovedb::QueryItem::Key({'k'})})
          .and_then([&](grovedb::PathQuery q) { return db->QueryRaw(q, /*result_type=*/2); });
  BOOST_REQUIRE(result.has_value());
  auto& elems = result->value().m_data;
  BOOST_CHECK_EQUAL(elems.size(), 1u);
  BOOST_CHECK(elems[0].m_kind == grovedb::QueryResultKind::PathKeyElementTrio);
  BOOST_CHECK(elems[0].m_key == grovedb::Bytes{'k'});
  BOOST_CHECK(elems[0].m_path.empty());
  BOOST_CHECK(!elems[0].m_element.empty());
}

BOOST_AUTO_TEST_CASE(query_many_raw)
{
  grovedb::test::TempDir dir("q_many_raw");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path root{};
  auto e1 = grovedb::Element::Item({'1'});
  auto e2 = grovedb::Element::Item({'2'});
  BOOST_REQUIRE(e1.has_value());
  BOOST_REQUIRE(e2.has_value());
  BOOST_REQUIRE(db->Put(root, {'a'}, *e1).has_value());
  BOOST_REQUIRE(db->Put(root, {'b'}, *e2).has_value());

  std::vector<grovedb::QueryItem> items_a{grovedb::QueryItem::Key({'a'})};
  std::vector<grovedb::QueryItem> items_b{grovedb::QueryItem::Key({'b'})};

  std::vector<grovedb::Db::RawQuerySpec> queries{
      {.path = root, .items = items_a, .limit = 0, .offset = 0},
      {.path = root, .items = items_b, .limit = 0, .offset = 0},
  };

  auto result = db->QueryManyRaw(queries, /*result_type=*/0);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK_EQUAL(result->value().size(), 2u);
}

BOOST_AUTO_TEST_CASE(query_keys_optional)
{
  grovedb::test::TempDir dir("q_keys_opt");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path root{};
  auto item = grovedb::Element::Item({'v'});
  BOOST_REQUIRE(item.has_value());
  BOOST_REQUIRE(db->Put(root, {'a'}, *item).has_value());

  auto result = grovedb::PathQuery::New(
                    root,
                    {grovedb::QueryItem::Key({'a'}), grovedb::QueryItem::Key({'z'})},
                    /*limit=*/100
  )
                    .and_then([&](grovedb::PathQuery q) { return db->QueryKeysOptional(q); });
  BOOST_REQUIRE(result.has_value());
  auto& entries = result->value();
  BOOST_CHECK_EQUAL(entries.size(), 2u);

  BOOST_CHECK(entries[0].m_key == grovedb::Bytes{'a'});
  BOOST_CHECK(entries[0].m_element.has_value());

  BOOST_CHECK(entries[1].m_key == grovedb::Bytes{'z'});
  BOOST_CHECK(!entries[1].m_element.has_value());
}

BOOST_AUTO_TEST_CASE(query_raw_keys_optional)
{
  grovedb::test::TempDir dir("q_raw_keys_opt");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path root{};
  auto item = grovedb::Element::Item({'v'});
  BOOST_REQUIRE(item.has_value());
  BOOST_REQUIRE(db->Put(root, {'x'}, *item).has_value());

  auto result = grovedb::PathQuery::New(
                    root,
                    {grovedb::QueryItem::Key({'x'}), grovedb::QueryItem::Key({'y'})},
                    /*limit=*/100
  )
                    .and_then([&](grovedb::PathQuery q) { return db->QueryRawKeysOptional(q); });
  BOOST_REQUIRE(result.has_value());
  auto& entries = result->value();
  BOOST_CHECK_EQUAL(entries.size(), 2u);

  BOOST_CHECK(entries[0].m_key == grovedb::Bytes{'x'});
  BOOST_CHECK(entries[0].m_element.has_value());

  BOOST_CHECK(entries[1].m_key == grovedb::Bytes{'y'});
  BOOST_CHECK(!entries[1].m_element.has_value());
}

BOOST_AUTO_TEST_CASE(deep_path_query)
{
  grovedb::test::TempDir dir("q_deep");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  // Create a chain of 10 nested subtrees.
  static constexpr int DEPTH = 10;
  grovedb::Path path{};
  for (int i = 0; i < DEPTH; ++i) {
    grovedb::Bytes key{'l', static_cast<uint8_t>(i)};
    auto tree = grovedb::Element::EmptyTree();
    BOOST_REQUIRE(tree.has_value());
    BOOST_REQUIRE(db->Put(path, key, *tree).has_value());
    path.push_back(key);
  }

  // Insert 3 items at the deepest level.
  grovedb::Bytes v1{'x'}, v2{'y'}, v3{'z'};
  auto e1 = grovedb::Element::Item(v1);
  auto e2 = grovedb::Element::Item(v2);
  auto e3 = grovedb::Element::Item(v3);
  BOOST_REQUIRE(e1.has_value());
  BOOST_REQUIRE(e2.has_value());
  BOOST_REQUIRE(e3.has_value());
  BOOST_REQUIRE(db->Put(path, {'a'}, *e1).has_value());
  BOOST_REQUIRE(db->Put(path, {'b'}, *e2).has_value());
  BOOST_REQUIRE(db->Put(path, {'c'}, *e3).has_value());

  // Query all values at the deepest path.
  auto result = grovedb::PathQuery::New(path, {grovedb::QueryItem::RangeFull()})
                    .and_then([&](grovedb::PathQuery q) { return db->QueryValues(q); });
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK_EQUAL(result->value().m_data.size(), 3u);
  BOOST_CHECK(result->value().m_data[0] == v1);
  BOOST_CHECK(result->value().m_data[1] == v2);
  BOOST_CHECK(result->value().m_data[2] == v3);
}

BOOST_AUTO_TEST_SUITE_END()

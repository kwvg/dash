// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <test/util/tempdir.h>

#include <grovedb/db.h>
#include <grovedb/element.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(insert_tests)

BOOST_AUTO_TEST_CASE(put_and_get_direct_round_trip)
{
  grovedb::test::TempDir dir("insert_round_trip");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path path{};
  grovedb::Bytes key{'k'};
  auto elem = grovedb::Element::Item(grovedb::Bytes::FromString("hello"));
  BOOST_REQUIRE(elem.has_value());

  BOOST_REQUIRE(db->Put(path, key, *elem).has_value());

  auto got = db->GetDirect(path, key);
  BOOST_REQUIRE(got.has_value());
  BOOST_CHECK(got->value() == *elem);
}

BOOST_AUTO_TEST_CASE(put_if_absent_inserts_when_missing)
{
  grovedb::test::TempDir dir("insert_absent_ok");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path path{};
  grovedb::Bytes key{'a'};
  auto elem = grovedb::Element::Item(grovedb::Bytes{'v'});
  BOOST_REQUIRE(elem.has_value());

  auto result = db->PutIfAbsent(path, key, *elem);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(result->value()); // was inserted
}

BOOST_AUTO_TEST_CASE(put_if_absent_skips_when_exists)
{
  grovedb::test::TempDir dir("insert_absent_dup");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path path{};
  grovedb::Bytes key{'b'};
  auto elem = grovedb::Element::Item(grovedb::Bytes{'v'});
  BOOST_REQUIRE(elem.has_value());
  BOOST_REQUIRE(db->Put(path, key, *elem).has_value());

  auto result = db->PutIfAbsent(path, key, *elem);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(!result->value()); // was not inserted
}

BOOST_AUTO_TEST_CASE(put_if_absent_and_get_returns_nullopt_when_inserted)
{
  grovedb::test::TempDir dir("insert_absent_get_new");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path path{};
  grovedb::Bytes key{'c'};
  auto elem = grovedb::Element::Item(grovedb::Bytes{'v'});
  BOOST_REQUIRE(elem.has_value());

  auto result = db->PutIfAbsentAndGet(path, key, *elem);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(!result->value().has_value()); // no existing element
}

BOOST_AUTO_TEST_CASE(put_if_absent_and_get_returns_existing)
{
  grovedb::test::TempDir dir("insert_absent_get_old");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path path{};
  grovedb::Bytes key{'d'};
  auto elem1 = grovedb::Element::Item(grovedb::Bytes::FromString("v1"));
  auto elem2 = grovedb::Element::Item(grovedb::Bytes::FromString("v2"));
  BOOST_REQUIRE(elem1.has_value());
  BOOST_REQUIRE(elem2.has_value());
  BOOST_REQUIRE(db->Put(path, key, *elem1).has_value());

  auto result = db->PutIfAbsentAndGet(path, key, *elem2);
  BOOST_REQUIRE(result.has_value());
  BOOST_REQUIRE(result->value().has_value());
  BOOST_CHECK(!result->value()->empty());
}

BOOST_AUTO_TEST_CASE(put_if_changed_new_key)
{
  grovedb::test::TempDir dir("insert_changed_new");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path path{};
  grovedb::Bytes key{'e'};
  auto elem = grovedb::Element::Item(grovedb::Bytes{'v'});
  BOOST_REQUIRE(elem.has_value());

  auto result = db->PutIfChanged(path, key, *elem);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(result->value().m_changed);
  BOOST_CHECK(!result->value().m_previous.has_value());
}

BOOST_AUTO_TEST_CASE(put_if_changed_same_value)
{
  grovedb::test::TempDir dir("insert_changed_same");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path path{};
  grovedb::Bytes key{'f'};
  auto elem = grovedb::Element::Item(grovedb::Bytes{'v'});
  BOOST_REQUIRE(elem.has_value());
  BOOST_REQUIRE(db->Put(path, key, *elem).has_value());

  auto result = db->PutIfChanged(path, key, *elem);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(!result->value().m_changed);
}

BOOST_AUTO_TEST_CASE(put_if_changed_different_value)
{
  grovedb::test::TempDir dir("insert_changed_diff");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path path{};
  grovedb::Bytes key{'g'};
  auto elem1 = grovedb::Element::Item(grovedb::Bytes::FromString("v1"));
  auto elem2 = grovedb::Element::Item(grovedb::Bytes::FromString("v2"));
  BOOST_REQUIRE(elem1.has_value());
  BOOST_REQUIRE(elem2.has_value());
  BOOST_REQUIRE(db->Put(path, key, *elem1).has_value());

  auto result = db->PutIfChanged(path, key, *elem2);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(result->value().m_changed);
  BOOST_REQUIRE(result->value().m_previous.has_value());
  BOOST_CHECK(!result->value().m_previous->empty());
}

BOOST_AUTO_TEST_CASE(deep_nested_insert_get)
{
  grovedb::test::TempDir dir("insert_deep");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  // Create a chain of 20 nested subtrees.
  static constexpr int DEPTH = 20;
  grovedb::Path path{};
  for (int i = 0; i < DEPTH; ++i) {
    grovedb::Bytes key{'l', static_cast<uint8_t>(i)};
    auto tree = grovedb::Element::EmptyTree();
    BOOST_REQUIRE(tree.has_value());
    BOOST_REQUIRE(db->Put(path, key, *tree).has_value());
    path.push_back(key);
  }

  // Insert an Item at the deepest level.
  grovedb::Bytes leaf_key{'v'};
  auto elem = grovedb::Element::Item(grovedb::Bytes::FromString("deep"));
  BOOST_REQUIRE(elem.has_value());
  BOOST_REQUIRE(db->Put(path, leaf_key, *elem).has_value());

  // Get it back and verify round-trip.
  auto got = db->Get(path, leaf_key);
  BOOST_REQUIRE(got.has_value());
  BOOST_CHECK(got->value() == *elem);

  // Cost should be non-trivial at depth 20.
  BOOST_CHECK(got->cost().m_seek_count > 0);
}

BOOST_AUTO_TEST_SUITE_END()

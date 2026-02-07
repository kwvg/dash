// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <test/util/tempdir.h>

#include <grovedb/db.h>
#include <grovedb/element.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(get_tests)

BOOST_AUTO_TEST_CASE(get_missing_key_returns_error)
{
  grovedb::test::TempDir dir("get_missing");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path path{};
  grovedb::Bytes key{'x'};
  auto result = db->Get(path, key);
  BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(get_existing_item)
{
  grovedb::test::TempDir dir("get_existing");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path path{};
  grovedb::Bytes key{'k'};
  auto elem = grovedb::Element::Item(grovedb::Bytes::FromString("val"));
  BOOST_REQUIRE(elem.has_value());
  BOOST_REQUIRE(db->Put(path, key, *elem).has_value());

  auto result = db->Get(path, key);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(!result->value().empty());
}

BOOST_AUTO_TEST_CASE(get_direct_existing_item)
{
  grovedb::test::TempDir dir("get_direct");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path path{};
  grovedb::Bytes key{'d'};
  auto elem = grovedb::Element::Item(grovedb::Bytes{'v'});
  BOOST_REQUIRE(elem.has_value());
  BOOST_REQUIRE(db->Put(path, key, *elem).has_value());

  auto result = db->GetDirect(path, key);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(!result->value().empty());
}

BOOST_AUTO_TEST_CASE(get_optional_missing_returns_nullopt)
{
  grovedb::test::TempDir dir("get_opt_missing");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path path{};
  grovedb::Bytes key{'m'};
  auto result = db->GetOptional(path, key);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(!result->value().has_value());
}

BOOST_AUTO_TEST_CASE(get_optional_existing_returns_element)
{
  grovedb::test::TempDir dir("get_opt_existing");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path path{};
  grovedb::Bytes key{'o'};
  auto elem = grovedb::Element::Item(grovedb::Bytes{'v'});
  BOOST_REQUIRE(elem.has_value());
  BOOST_REQUIRE(db->Put(path, key, *elem).has_value());

  auto result = db->GetOptional(path, key);
  BOOST_REQUIRE(result.has_value());
  BOOST_REQUIRE(result->value().has_value());
  BOOST_CHECK(!result->value()->empty());
}

BOOST_AUTO_TEST_CASE(key_exists_false_for_missing)
{
  grovedb::test::TempDir dir("key_exists_f");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path path{};
  grovedb::Bytes key{'n'};
  auto result = db->KeyExists(path, key);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(!result->value());
}

BOOST_AUTO_TEST_CASE(key_exists_true_after_insert)
{
  grovedb::test::TempDir dir("key_exists_t");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path path{};
  grovedb::Bytes key{'e'};
  auto elem = grovedb::Element::Item(grovedb::Bytes{'v'});
  BOOST_REQUIRE(elem.has_value());
  BOOST_REQUIRE(db->Put(path, key, *elem).has_value());

  auto result = db->KeyExists(path, key);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(result->value());
}

BOOST_AUTO_TEST_CASE(subtree_exists_root)
{
  grovedb::test::TempDir dir("subtree_root");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path path{};
  auto result = db->SubtreeExists(path);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(result->value());
}

BOOST_AUTO_TEST_CASE(subtree_exists_missing)
{
  grovedb::test::TempDir dir("subtree_miss");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path path{grovedb::Bytes::FromString("no")};
  auto result = db->SubtreeExists(path);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(!result->value());
}

BOOST_AUTO_TEST_CASE(is_empty_tree_on_fresh_db)
{
  grovedb::test::TempDir dir("empty_root");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path path{};
  auto result = db->IsEmptyTree(path);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(result->value());
}

BOOST_AUTO_TEST_CASE(is_empty_tree_false_after_insert)
{
  grovedb::test::TempDir dir("empty_after");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path path{};
  grovedb::Bytes key{'k'};
  auto elem = grovedb::Element::Item(grovedb::Bytes{'v'});
  BOOST_REQUIRE(elem.has_value());
  BOOST_REQUIRE(db->Put(path, key, *elem).has_value());

  auto result = db->IsEmptyTree(path);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(!result->value());
}

BOOST_AUTO_TEST_SUITE_END()

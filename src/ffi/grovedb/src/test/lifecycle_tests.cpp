// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <test/util/tempdir.h>

#include <grovedb/db.h>

#include <boost/test/unit_test.hpp>

#include <string>

BOOST_AUTO_TEST_SUITE(lifecycle_tests)

BOOST_AUTO_TEST_CASE(open_and_close)
{
  grovedb::test::TempDir dir("lifecycle_open_close");
  auto result = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(result.has_value());
}

BOOST_AUTO_TEST_CASE(flush)
{
  grovedb::test::TempDir dir("lifecycle_flush");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());
  auto result = db->Flush();
  BOOST_CHECK(result.has_value());
}

BOOST_AUTO_TEST_CASE(destroy)
{
  grovedb::test::TempDir dir("lifecycle_destroy");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());
  auto result = db->Destroy();
  BOOST_CHECK(result.has_value());
}

BOOST_AUTO_TEST_CASE(root_hash_is_32_bytes)
{
  grovedb::test::TempDir dir("lifecycle_root_hash");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());
  auto result = db->GetRootHash();
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK_EQUAL(result->value().size(), 32u);
}

BOOST_AUTO_TEST_CASE(verify_integrity)
{
  grovedb::test::TempDir dir("lifecycle_verify");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());
  auto result = db->VerifyIntegrity();
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(*result);
}

BOOST_AUTO_TEST_CASE(whoami)
{
  auto result = grovedb::GetWhoami();
  BOOST_CHECK(!result.empty());
  BOOST_CHECK(result.find("grovedb") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

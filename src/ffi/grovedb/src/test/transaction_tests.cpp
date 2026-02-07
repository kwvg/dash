// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <test/util/tempdir.h>

#include <grovedb/db.h>
#include <grovedb/element.h>

#include <boost/test/unit_test.hpp>

#include <utility>

BOOST_AUTO_TEST_SUITE(transaction_tests)

BOOST_AUTO_TEST_CASE(begin_and_commit)
{
  grovedb::test::TempDir dir("tx_begin_commit");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());
  auto tx = db->BeginTransaction();
  BOOST_REQUIRE(tx.has_value());
  auto cost = db->Commit(*tx);
  BOOST_CHECK(cost.has_value());
}

BOOST_AUTO_TEST_CASE(begin_and_rollback)
{
  grovedb::test::TempDir dir("tx_begin_rollback");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());
  auto tx = db->BeginTransaction();
  BOOST_REQUIRE(tx.has_value());
  auto result = db->Rollback(*tx);
  BOOST_CHECK(result.has_value());
}

BOOST_AUTO_TEST_CASE(move_semantics)
{
  grovedb::test::TempDir dir("tx_move");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());
  auto tx = db->BeginTransaction();
  BOOST_REQUIRE(tx.has_value());
  grovedb::Transaction moved = std::move(*tx);
  auto cost = db->Commit(moved);
  BOOST_CHECK(cost.has_value());
}

BOOST_AUTO_TEST_CASE(auto_rollback_on_destruction)
{
  grovedb::test::TempDir dir("tx_auto_rollback");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());
  {
    auto tx = db->BeginTransaction();
    BOOST_REQUIRE(tx.has_value());
    // tx goes out of scope — destructor auto-rolls-back
  }
  // Verify the database is still accessible after auto-rollback.
  auto hash = db->GetRootHash();
  BOOST_CHECK(hash.has_value());
}

BOOST_AUTO_TEST_CASE(insert_visible_within_transaction)
{
  grovedb::test::TempDir dir("tx_insert_visible");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path path{};
  auto key = grovedb::Bytes::FromString("txk");
  auto elem = grovedb::Element::Item(grovedb::Bytes{'v'});
  BOOST_REQUIRE(elem.has_value());

  auto tx = db->BeginTransaction();
  BOOST_REQUIRE(tx.has_value());
  BOOST_REQUIRE(db->Put(path, key, *elem, *tx).has_value());

  // Key should be visible within the transaction.
  auto exists = db->KeyExists(path, key, *tx);
  BOOST_REQUIRE(exists.has_value());
  BOOST_CHECK(exists->value());

  BOOST_REQUIRE(db->Commit(*tx).has_value());
}

BOOST_AUTO_TEST_CASE(rollback_discards_insert)
{
  grovedb::test::TempDir dir("tx_rollback_discard");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path path{};
  auto key = grovedb::Bytes::FromString("rbk");
  auto elem = grovedb::Element::Item(grovedb::Bytes{'v'});
  BOOST_REQUIRE(elem.has_value());

  auto tx = db->BeginTransaction();
  BOOST_REQUIRE(tx.has_value());
  BOOST_REQUIRE(db->Put(path, key, *elem, *tx).has_value());
  BOOST_REQUIRE(db->Rollback(*tx).has_value());

  // Key should not be visible after rollback.
  auto exists = db->KeyExists(path, key);
  BOOST_REQUIRE(exists.has_value());
  BOOST_CHECK(!exists->value());
}

BOOST_AUTO_TEST_SUITE_END()

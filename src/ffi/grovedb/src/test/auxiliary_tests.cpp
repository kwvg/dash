// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <test/util/tempdir.h>

#include <grovedb/db.h>
#include <grovedb/element.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(auxiliary_tests)

BOOST_AUTO_TEST_CASE(put_aux_and_get_aux_round_trip)
{
  grovedb::test::TempDir dir("aux_round_trip");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  auto key = grovedb::Bytes::FromString("ak");
  auto value = grovedb::Bytes::FromString("av");
  BOOST_REQUIRE(db->PutAux(key, value).has_value());

  auto result = db->GetAux(key);
  BOOST_REQUIRE(result.has_value());
  BOOST_REQUIRE(result->value().has_value());
  BOOST_CHECK(*result->value() == value);
}

BOOST_AUTO_TEST_CASE(get_aux_missing_returns_nullopt)
{
  grovedb::test::TempDir dir("aux_get_missing");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  auto key = grovedb::Bytes::FromString("miss");
  auto result = db->GetAux(key);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(!result->value().has_value());
}

BOOST_AUTO_TEST_CASE(delete_aux_removes_value)
{
  grovedb::test::TempDir dir("aux_delete");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  auto key = grovedb::Bytes::FromString("dk");
  auto value = grovedb::Bytes::FromString("dv");
  BOOST_REQUIRE(db->PutAux(key, value).has_value());

  // Verify it's there.
  auto before = db->GetAux(key);
  BOOST_REQUIRE(before.has_value());
  BOOST_REQUIRE(before->value().has_value());

  // Delete it.
  BOOST_REQUIRE(db->DeleteAux(key).has_value());

  // Verify it's gone.
  auto after = db->GetAux(key);
  BOOST_REQUIRE(after.has_value());
  BOOST_CHECK(!after->value().has_value());
}

BOOST_AUTO_TEST_CASE(put_aux_with_transaction_commit)
{
  grovedb::test::TempDir dir("aux_tx_commit");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  auto key = grovedb::Bytes::FromString("tk");
  auto value = grovedb::Bytes::FromString("tv");

  auto tx = db->BeginTransaction();
  BOOST_REQUIRE(tx.has_value());
  BOOST_REQUIRE(db->PutAux(key, value, *tx).has_value());

  // Visible within the transaction.
  auto in_tx = db->GetAux(key, *tx);
  BOOST_REQUIRE(in_tx.has_value());
  BOOST_REQUIRE(in_tx->value().has_value());
  BOOST_CHECK(*in_tx->value() == value);

  BOOST_REQUIRE(db->Commit(*tx).has_value());

  // Still visible after commit.
  auto after = db->GetAux(key);
  BOOST_REQUIRE(after.has_value());
  BOOST_REQUIRE(after->value().has_value());
  BOOST_CHECK(*after->value() == value);
}

BOOST_AUTO_TEST_CASE(put_aux_with_transaction_rollback)
{
  grovedb::test::TempDir dir("aux_tx_rollback");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  auto key = grovedb::Bytes::FromString("rk");
  auto value = grovedb::Bytes::FromString("rv");

  auto tx = db->BeginTransaction();
  BOOST_REQUIRE(tx.has_value());
  BOOST_REQUIRE(db->PutAux(key, value, *tx).has_value());
  BOOST_REQUIRE(db->Rollback(*tx).has_value());

  // Should not be visible after rollback.
  auto after = db->GetAux(key);
  BOOST_REQUIRE(after.has_value());
  BOOST_CHECK(!after->value().has_value());
}

BOOST_AUTO_TEST_CASE(delete_aux_with_transaction)
{
  grovedb::test::TempDir dir("aux_delete_tx");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  auto key = grovedb::Bytes::FromString("xk");
  auto value = grovedb::Bytes::FromString("xv");
  BOOST_REQUIRE(db->PutAux(key, value).has_value());

  auto tx = db->BeginTransaction();
  BOOST_REQUIRE(tx.has_value());
  BOOST_REQUIRE(db->DeleteAux(key, *tx).has_value());
  BOOST_REQUIRE(db->Commit(*tx).has_value());

  auto after = db->GetAux(key);
  BOOST_REQUIRE(after.has_value());
  BOOST_CHECK(!after->value().has_value());
}

BOOST_AUTO_TEST_CASE(find_subtrees_empty_db)
{
  grovedb::test::TempDir dir("aux_find_empty");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path root{};
  auto result = db->FindSubtrees(root);
  BOOST_REQUIRE(result.has_value());
  // An empty database should return at least the root path.
  BOOST_CHECK(!result->value().empty());
}

BOOST_AUTO_TEST_CASE(find_subtrees_after_insert)
{
  grovedb::test::TempDir dir("aux_find_after");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  // Create a subtree at the root.
  grovedb::Path root{};
  auto subtree_key = grovedb::Bytes::FromString("sub");
  auto tree = grovedb::Element::EmptyTree();
  BOOST_REQUIRE(tree.has_value());
  BOOST_REQUIRE(db->Put(root, subtree_key, *tree).has_value());

  auto result = db->FindSubtrees(root);
  BOOST_REQUIRE(result.has_value());
  // Should find at least root + the new subtree.
  BOOST_CHECK_GE(result->value().size(), 2u);
}

BOOST_AUTO_TEST_CASE(find_subtrees_with_transaction)
{
  grovedb::test::TempDir dir("aux_find_tx");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path root{};
  auto subtree_key = grovedb::Bytes::FromString("txs");
  auto tree = grovedb::Element::EmptyTree();
  BOOST_REQUIRE(tree.has_value());

  auto tx = db->BeginTransaction();
  BOOST_REQUIRE(tx.has_value());
  BOOST_REQUIRE(db->Put(root, subtree_key, *tree, *tx).has_value());

  auto result = db->FindSubtrees(root, *tx);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK_GE(result->value().size(), 2u);

  BOOST_REQUIRE(db->Commit(*tx).has_value());
}

BOOST_AUTO_TEST_CASE(put_aux_multiple_keys)
{
  grovedb::test::TempDir dir("aux_multi");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  auto key1 = grovedb::Bytes::FromString("k1");
  auto val1 = grovedb::Bytes::FromString("v1");
  auto key2 = grovedb::Bytes::FromString("k2");
  auto val2 = grovedb::Bytes::FromString("v2");

  BOOST_REQUIRE(db->PutAux(key1, val1).has_value());
  BOOST_REQUIRE(db->PutAux(key2, val2).has_value());

  auto r1 = db->GetAux(key1);
  BOOST_REQUIRE(r1.has_value());
  BOOST_REQUIRE(r1->value().has_value());
  BOOST_CHECK(*r1->value() == val1);

  auto r2 = db->GetAux(key2);
  BOOST_REQUIRE(r2.has_value());
  BOOST_REQUIRE(r2->value().has_value());
  BOOST_CHECK(*r2->value() == val2);
}

BOOST_AUTO_TEST_CASE(put_aux_overwrites_existing)
{
  grovedb::test::TempDir dir("aux_overwrite");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  auto key = grovedb::Bytes::FromString("ok");
  auto val1 = grovedb::Bytes::FromString("v1");
  auto val2 = grovedb::Bytes::FromString("v2");

  BOOST_REQUIRE(db->PutAux(key, val1).has_value());
  BOOST_REQUIRE(db->PutAux(key, val2).has_value());

  auto result = db->GetAux(key);
  BOOST_REQUIRE(result.has_value());
  BOOST_REQUIRE(result->value().has_value());
  BOOST_CHECK(*result->value() == val2);
}

BOOST_AUTO_TEST_SUITE_END()

// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <test/util/tempdir.h>

#include <grovedb/db.h>
#include <grovedb/element.h>
#include <grovedb/query.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>

BOOST_AUTO_TEST_SUITE(proof_tests)

namespace {
// Helper: open a database and insert 3 items under root.
grovedb::Db SetupDb(grovedb::test::TempDir& dir)
{
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());
  grovedb::Path root{};
  for (uint8_t i = 0; i < 3; ++i) {
    auto elem = grovedb::Element::Item({static_cast<uint8_t>('1' + i)});
    BOOST_REQUIRE(elem.has_value());
    BOOST_REQUIRE(db->Put(root, {static_cast<uint8_t>('a' + i)}, *elem).has_value());
  }
  return std::move(*db);
}
} // anonymous namespace

BOOST_AUTO_TEST_CASE(prove_and_verify_round_trip)
{
  grovedb::test::TempDir dir("pf_round_trip");
  auto db = SetupDb(dir);

  // Build a query for all items.
  grovedb::Path root{};
  auto query = grovedb::PathQuery::New(root, {grovedb::QueryItem::RangeFull()}, /*limit=*/10);
  BOOST_REQUIRE(query.has_value());

  // Prove.
  auto proof_result = db.Prove(*query);
  BOOST_REQUIRE(proof_result.has_value());
  auto& proof_bytes = proof_result->value();
  BOOST_CHECK(!proof_bytes.empty());

  // Verify.
  auto verify_result = db.VerifyQuery(proof_bytes, *query);
  BOOST_REQUIRE(verify_result.has_value());
  auto& vr = verify_result->value();

  // Root hash should be 32 bytes.
  bool all_zero =
      std::all_of(vr.m_root_hash.begin(), vr.m_root_hash.end(), [](uint8_t b) { return b == 0; });
  BOOST_CHECK(!all_zero);

  // Should have 3 entries.
  BOOST_CHECK_EQUAL(vr.m_entries.size(), 3u);
}

BOOST_AUTO_TEST_CASE(prove_root_hash_matches_db)
{
  grovedb::test::TempDir dir("pf_root_hash");
  auto db = SetupDb(dir);

  grovedb::Path root{};
  auto query = grovedb::PathQuery::New(root, {grovedb::QueryItem::RangeFull()}, /*limit=*/10);
  BOOST_REQUIRE(query.has_value());

  auto proof_result = db.Prove(*query);
  BOOST_REQUIRE(proof_result.has_value());

  auto verify_result = db.VerifyQuery(proof_result->value(), *query);
  BOOST_REQUIRE(verify_result.has_value());

  // The verified root hash should match the database root hash.
  auto db_root = db.GetRootHash();
  BOOST_REQUIRE(db_root.has_value());
  BOOST_CHECK(verify_result->value().m_root_hash == db_root->value());
}

BOOST_AUTO_TEST_CASE(verify_subset_query)
{
  grovedb::test::TempDir dir("pf_subset");
  auto db = SetupDb(dir);

  grovedb::Path root{};
  // Prove with RangeFull.
  auto query = grovedb::PathQuery::New(root, {grovedb::QueryItem::RangeFull()}, /*limit=*/10);
  BOOST_REQUIRE(query.has_value());
  auto proof_result = db.Prove(*query);
  BOOST_REQUIRE(proof_result.has_value());

  // Verify with a subset query (single key 'b').
  auto subset_query = grovedb::PathQuery::New(root, {grovedb::QueryItem::Key({'b'})}, /*limit=*/10);
  BOOST_REQUIRE(subset_query.has_value());

  auto verify_result = db.VerifySubsetQuery(proof_result->value(), *subset_query);
  BOOST_REQUIRE(verify_result.has_value());
  BOOST_CHECK_EQUAL(verify_result->value().m_entries.size(), 1u);
}

BOOST_AUTO_TEST_CASE(verify_query_with_absence_proof)
{
  grovedb::test::TempDir dir("pf_absence");
  auto db = SetupDb(dir);

  grovedb::Path root{};
  // Query for a specific key that exists + one that doesn't.
  auto query = grovedb::PathQuery::New(
      root, {grovedb::QueryItem::Key({'a'}), grovedb::QueryItem::Key({'z'})}, /*limit=*/10
  );
  BOOST_REQUIRE(query.has_value());

  auto proof_result = db.Prove(*query);
  BOOST_REQUIRE(proof_result.has_value());

  auto verify_result = db.VerifyQueryWithAbsenceProof(proof_result->value(), *query);
  BOOST_REQUIRE(verify_result.has_value());

  // Should have entries — 'a' exists (with element), 'z' absent (nullopt).
  auto& entries = verify_result->value().m_entries;
  BOOST_CHECK(!entries.empty());

  // Find the 'a' entry — should have element.
  bool found_a = false;
  for (const auto& e : entries) {
    if (e.m_key == grovedb::Bytes{'a'}) {
      BOOST_CHECK(e.m_element.has_value());
      found_a = true;
    }
  }
  BOOST_CHECK(found_a);
}

BOOST_AUTO_TEST_CASE(verify_query_with_custom_options)
{
  grovedb::test::TempDir dir("pf_options");
  auto db = SetupDb(dir);

  grovedb::Path root{};
  auto query = grovedb::PathQuery::New(root, {grovedb::QueryItem::RangeFull()}, /*limit=*/10);
  BOOST_REQUIRE(query.has_value());

  auto proof_result = db.Prove(*query);
  BOOST_REQUIRE(proof_result.has_value());

  // Verify with custom options (no absence proofs, no succinctness, no empty trees).
  auto verify_result = db.VerifyQueryWithOptions(
      proof_result->value(),
      *query,
      /*absence_proofs=*/false,
      /*verify_succinctness=*/false,
      /*include_empty_trees=*/false
  );
  BOOST_REQUIRE(verify_result.has_value());
  BOOST_CHECK_EQUAL(verify_result->value().m_entries.size(), 3u);
}

BOOST_AUTO_TEST_CASE(corrupt_proof_detected)
{
  grovedb::test::TempDir dir("pf_corrupt");
  auto db = SetupDb(dir);

  grovedb::Path root{};
  auto query = grovedb::PathQuery::New(root, {grovedb::QueryItem::RangeFull()}, /*limit=*/10);
  BOOST_REQUIRE(query.has_value());

  auto proof_result = db.Prove(*query);
  BOOST_REQUIRE(proof_result.has_value());

  // Corrupt the proof by flipping some bytes.
  auto corrupt_proof = proof_result->value();
  if (corrupt_proof.size() > 10) {
    corrupt_proof[5] ^= 0xFF;
    corrupt_proof[10] ^= 0xFF;
  }

  // Verification should fail.
  auto verify_result = db.VerifyQuery(corrupt_proof, *query);
  BOOST_CHECK(!verify_result.has_value());
}

BOOST_AUTO_TEST_CASE(prove_single_key)
{
  grovedb::test::TempDir dir("pf_single");
  auto db = SetupDb(dir);

  grovedb::Path root{};
  auto query = grovedb::PathQuery::New(root, {grovedb::QueryItem::Key({'b'})}, /*limit=*/10);
  BOOST_REQUIRE(query.has_value());

  auto proof_result = db.Prove(*query);
  BOOST_REQUIRE(proof_result.has_value());

  auto verify_result = db.VerifyQuery(proof_result->value(), *query);
  BOOST_REQUIRE(verify_result.has_value());
  BOOST_CHECK_EQUAL(verify_result->value().m_entries.size(), 1u);

  // The entry should have key 'b' with an element present.
  auto& entry = verify_result->value().m_entries[0];
  BOOST_CHECK(entry.m_key == grovedb::Bytes{'b'});
  BOOST_CHECK(entry.m_element.has_value());
}

BOOST_AUTO_TEST_CASE(prove_in_subtree)
{
  grovedb::test::TempDir dir("pf_subtree");
  auto db_r = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db_r.has_value());
  auto& db = *db_r;

  grovedb::Path root{};
  grovedb::Bytes sub_key{'s'};

  // Create subtree and insert items.
  auto tree = grovedb::Element::EmptyTree();
  BOOST_REQUIRE(tree.has_value());
  BOOST_REQUIRE(db.Put(root, sub_key, *tree).has_value());

  grovedb::Path sub_path{sub_key};
  for (uint8_t i = 0; i < 2; ++i) {
    auto elem = grovedb::Element::Item({static_cast<uint8_t>('x' + i)});
    BOOST_REQUIRE(elem.has_value());
    BOOST_REQUIRE(db.Put(sub_path, {static_cast<uint8_t>('k' + i)}, *elem).has_value());
  }

  // Prove a query in the subtree.
  auto query = grovedb::PathQuery::New(sub_path, {grovedb::QueryItem::RangeFull()}, /*limit=*/10);
  BOOST_REQUIRE(query.has_value());

  auto proof_result = db.Prove(*query);
  BOOST_REQUIRE(proof_result.has_value());

  auto verify_result = db.VerifyQuery(proof_result->value(), *query);
  BOOST_REQUIRE(verify_result.has_value());
  BOOST_CHECK_EQUAL(verify_result->value().m_entries.size(), 2u);
}

BOOST_AUTO_TEST_SUITE_END()

// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <grovedb/db.h>

#include <test/util/tempdir.h>

#include <algorithm>
#include <cstdint>
#include <vector>

BOOST_AUTO_TEST_SUITE(proof_tests)

// ---------------------------------------------------------------------------
// Prove basic
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_prove_query_basic)
{
    grovedb::test::TempDir tmp{"grovedb_test_prove_basic"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Insert an item.
    grovedb::Element item;
    BOOST_REQUIRE(grovedb::Element::Item({'v', 'a', 'l'}, item).ok());
    BOOST_REQUIRE(db.Put(root, {'k'}, item, cost).ok());

    // Build a query for that key.
    grovedb::PathQuery query;
    BOOST_REQUIRE(grovedb::PathQuery::New(
        root,
        {grovedb::QueryItem::Key({'k'})},
        0, 0,
        query).ok());

    // Generate a proof.
    grovedb::Bytes proof;
    auto status = db.Prove(query, proof, cost);
    BOOST_CHECK_MESSAGE(status.ok(), status.message());
    BOOST_CHECK(!proof.empty());
    BOOST_CHECK(cost.m_seek_count > 0);
}

// ---------------------------------------------------------------------------
// Prove + Verify roundtrip
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_prove_verify_roundtrip)
{
    grovedb::test::TempDir tmp{"grovedb_test_prove_verify_rt"};
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

    // Query all keys.
    grovedb::PathQuery query;
    BOOST_REQUIRE(grovedb::PathQuery::New(
        root,
        {grovedb::QueryItem::RangeFull()},
        0, 0,
        query).ok());

    // Prove.
    grovedb::Bytes proof;
    BOOST_REQUIRE(db.Prove(query, proof, cost).ok());
    BOOST_CHECK(!proof.empty());

    // Verify.
    grovedb::Hash root_hash{};
    std::vector<grovedb::ProofResultEntry> entries;
    auto status = grovedb::Db::VerifyQuery(proof, query, root_hash, entries);
    BOOST_CHECK_MESSAGE(status.ok(), status.message());

    // Root hash should be non-zero.
    bool all_zero = std::all_of(root_hash.begin(), root_hash.end(),
                                [](uint8_t b) { return b == 0; });
    BOOST_CHECK(!all_zero);

    // Should have 3 entries, each with an element.
    BOOST_CHECK_EQUAL(entries.size(), 3);
    for (const auto& entry : entries) {
        BOOST_CHECK(entry.m_element.has_value());
        BOOST_CHECK(!entry.m_element->data().empty());
    }
}

// ---------------------------------------------------------------------------
// Root hash matches GetRootHash
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_verify_root_hash_matches)
{
    grovedb::test::TempDir tmp{"grovedb_test_verify_root_hash"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    grovedb::Element item;
    BOOST_REQUIRE(grovedb::Element::Item({'v'}, item).ok());
    BOOST_REQUIRE(db.Put(root, {'k'}, item, cost).ok());

    // Get the root hash from the database.
    grovedb::Hash expected_hash{};
    BOOST_REQUIRE(db.GetRootHash(expected_hash, cost).ok());

    // Prove and verify.
    grovedb::PathQuery query;
    BOOST_REQUIRE(grovedb::PathQuery::New(
        root,
        {grovedb::QueryItem::Key({'k'})},
        0, 0,
        query).ok());

    grovedb::Bytes proof;
    BOOST_REQUIRE(db.Prove(query, proof, cost).ok());

    grovedb::Hash verified_hash{};
    std::vector<grovedb::ProofResultEntry> entries;
    BOOST_REQUIRE(grovedb::Db::VerifyQuery(proof, query, verified_hash, entries).ok());

    BOOST_CHECK(verified_hash == expected_hash);
}

// ---------------------------------------------------------------------------
// Prove with options
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_prove_with_options)
{
    grovedb::test::TempDir tmp{"grovedb_test_prove_opts"};
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

    // Prove with decrease_limit_on_empty = false.
    grovedb::ProveOptions options{.m_decrease_limit_on_empty = false};
    grovedb::Bytes proof;
    auto status = db.Prove(query, options, proof, cost);
    BOOST_CHECK_MESSAGE(status.ok(), status.message());
    BOOST_CHECK(!proof.empty());

    // Verify still succeeds.
    grovedb::Hash root_hash{};
    std::vector<grovedb::ProofResultEntry> entries;
    BOOST_CHECK(grovedb::Db::VerifyQuery(proof, query, root_hash, entries).ok());
    BOOST_CHECK_EQUAL(entries.size(), 1);
}

// ---------------------------------------------------------------------------
// Verify with explicit options
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_verify_with_options)
{
    grovedb::test::TempDir tmp{"grovedb_test_verify_opts"};
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

    grovedb::Bytes proof;
    BOOST_REQUIRE(db.Prove(query, proof, cost).ok());

    // Verify with explicit options (absence proofs disabled since the query
    // has no limit, and GroveDB requires limits with absence proofs).
    grovedb::VerifyOptions options{
        .m_absence_proofs = false,
        .m_verify_succinctness = true,
        .m_include_empty_trees = false,
    };
    grovedb::Hash root_hash{};
    std::vector<grovedb::ProofResultEntry> entries;
    auto status = grovedb::Db::VerifyQuery(proof, query, options, root_hash, entries);
    BOOST_CHECK_MESSAGE(status.ok(), status.message());
    BOOST_CHECK_EQUAL(entries.size(), 1);
    BOOST_CHECK(entries[0].m_element.has_value());
}

// ---------------------------------------------------------------------------
// Verify subset query
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_verify_subset_query)
{
    grovedb::test::TempDir tmp{"grovedb_test_verify_subset"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Insert items at keys a-e.
    grovedb::Element item;
    for (uint8_t k = 'a'; k <= 'e'; ++k) {
        BOOST_REQUIRE(grovedb::Element::Item({k}, item).ok());
        BOOST_REQUIRE(db.Put(root, {k}, item, cost).ok());
    }

    // Prove with RangeFull (all keys).
    grovedb::PathQuery full_query;
    BOOST_REQUIRE(grovedb::PathQuery::New(
        root,
        {grovedb::QueryItem::RangeFull()},
        0, 0,
        full_query).ok());

    grovedb::Bytes proof;
    BOOST_REQUIRE(db.Prove(full_query, proof, cost).ok());

    // Verify with a subset query (single key 'c').
    grovedb::PathQuery subset_query;
    BOOST_REQUIRE(grovedb::PathQuery::New(
        root,
        {grovedb::QueryItem::Key({'c'})},
        0, 0,
        subset_query).ok());

    grovedb::Hash root_hash{};
    std::vector<grovedb::ProofResultEntry> entries;
    auto status = grovedb::Db::VerifySubsetQuery(proof, subset_query, root_hash, entries);
    BOOST_CHECK_MESSAGE(status.ok(), status.message());
    BOOST_CHECK_EQUAL(entries.size(), 1);
    BOOST_CHECK(entries[0].m_key == grovedb::Bytes{'c'});
}

// ---------------------------------------------------------------------------
// Verify query with absence proof
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_verify_query_with_absence_proof)
{
    grovedb::test::TempDir tmp{"grovedb_test_verify_absence"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Insert some items but NOT key 'z'.
    grovedb::Element item;
    BOOST_REQUIRE(grovedb::Element::Item({'v'}, item).ok());
    BOOST_REQUIRE(db.Put(root, {'a'}, item, cost).ok());

    // Query for the non-existent key 'z'.
    // Absence proof verification requires limits to be set on the query.
    grovedb::PathQuery query;
    BOOST_REQUIRE(grovedb::PathQuery::New(
        root,
        {grovedb::QueryItem::Key({'z'})},
        100, 0,
        query).ok());

    grovedb::Bytes proof;
    BOOST_REQUIRE(db.Prove(query, proof, cost).ok());

    grovedb::Hash root_hash{};
    std::vector<grovedb::ProofResultEntry> entries;
    auto status = grovedb::Db::VerifyQueryWithAbsenceProof(
        proof, query, root_hash, entries);
    BOOST_CHECK_MESSAGE(status.ok(), status.message());

    // The entry for 'z' should have no element (absence proof).
    BOOST_CHECK_EQUAL(entries.size(), 1);
    BOOST_CHECK(!entries[0].m_element.has_value());
}

// ---------------------------------------------------------------------------
// Verify subset with absence proof
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_verify_subset_with_absence_proof)
{
    grovedb::test::TempDir tmp{"grovedb_test_verify_subset_absence"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    grovedb::Element item;
    BOOST_REQUIRE(grovedb::Element::Item({'1'}, item).ok());
    BOOST_REQUIRE(db.Put(root, {'a'}, item, cost).ok());
    BOOST_REQUIRE(grovedb::Element::Item({'2'}, item).ok());
    BOOST_REQUIRE(db.Put(root, {'b'}, item, cost).ok());

    // Prove RangeFull with a limit (absence proof verification requires limits).
    grovedb::PathQuery full_query;
    BOOST_REQUIRE(grovedb::PathQuery::New(
        root,
        {grovedb::QueryItem::RangeFull()},
        100, 0,
        full_query).ok());

    grovedb::Bytes proof;
    BOOST_REQUIRE(db.Prove(full_query, proof, cost).ok());

    // Verify a subset with a key that exists ('a') as absence proof.
    // Absence proof verification requires limits to be set on the query.
    grovedb::PathQuery subset_query;
    BOOST_REQUIRE(grovedb::PathQuery::New(
        root,
        {grovedb::QueryItem::Key({'a'})},
        100, 0,
        subset_query).ok());

    grovedb::Hash root_hash{};
    std::vector<grovedb::ProofResultEntry> entries;
    auto status = grovedb::Db::VerifySubsetQueryWithAbsenceProof(
        proof, subset_query, root_hash, entries);
    BOOST_CHECK_MESSAGE(status.ok(), status.message());
    BOOST_CHECK_EQUAL(entries.size(), 1);
    BOOST_CHECK(entries[0].m_element.has_value());
}

// ---------------------------------------------------------------------------
// Prove empty result
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_prove_empty_result)
{
    grovedb::test::TempDir tmp{"grovedb_test_prove_empty"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Insert something so the tree is non-empty.
    grovedb::Element item;
    BOOST_REQUIRE(grovedb::Element::Item({'v'}, item).ok());
    BOOST_REQUIRE(db.Put(root, {'a'}, item, cost).ok());

    // Query for a key that doesn't exist.
    grovedb::PathQuery query;
    BOOST_REQUIRE(grovedb::PathQuery::New(
        root,
        {grovedb::QueryItem::Key({'z'})},
        0, 0,
        query).ok());

    grovedb::Bytes proof;
    auto status = db.Prove(query, proof, cost);
    BOOST_CHECK_MESSAGE(status.ok(), status.message());
    BOOST_CHECK(!proof.empty());
}

// ---------------------------------------------------------------------------
// Verify invalid proof
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_verify_invalid_proof)
{
    grovedb::test::TempDir tmp{"grovedb_test_verify_invalid"};
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

    // Pass garbage as a proof.
    grovedb::Bytes garbage{0xDE, 0xAD, 0xBE, 0xEF};
    grovedb::Hash root_hash{};
    std::vector<grovedb::ProofResultEntry> entries;
    auto status = grovedb::Db::VerifyQuery(garbage, query, root_hash, entries);
    BOOST_CHECK(!status.ok());
}

// ---------------------------------------------------------------------------
// Prove subtree
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_prove_subtree)
{
    grovedb::test::TempDir tmp{"grovedb_test_prove_subtree"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Create a subtree "t" and put items inside.
    grovedb::Element tree;
    BOOST_REQUIRE(grovedb::Element::EmptyTree(tree).ok());
    BOOST_REQUIRE(db.Put(root, {'t'}, tree, cost).ok());

    grovedb::Path subtree{{'t'}};
    grovedb::Element item;
    BOOST_REQUIRE(grovedb::Element::Item({'x'}, item).ok());
    BOOST_REQUIRE(db.Put(subtree, {'k'}, item, cost).ok());

    // Query inside the subtree.
    grovedb::PathQuery query;
    BOOST_REQUIRE(grovedb::PathQuery::New(
        subtree,
        {grovedb::QueryItem::Key({'k'})},
        0, 0,
        query).ok());

    grovedb::Bytes proof;
    BOOST_REQUIRE(db.Prove(query, proof, cost).ok());

    grovedb::Hash root_hash{};
    std::vector<grovedb::ProofResultEntry> entries;
    auto status = grovedb::Db::VerifyQuery(proof, query, root_hash, entries);
    BOOST_CHECK_MESSAGE(status.ok(), status.message());
    BOOST_CHECK_EQUAL(entries.size(), 1);
    BOOST_CHECK(entries[0].m_key == grovedb::Bytes{'k'});
    // Path should include the subtree segment.
    BOOST_CHECK_EQUAL(entries[0].m_path.size(), 1);
    BOOST_CHECK(entries[0].m_path[0] == grovedb::Bytes{'t'});
}

// ---------------------------------------------------------------------------
// Verify entry elements match
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_verify_entry_elements)
{
    grovedb::test::TempDir tmp{"grovedb_test_verify_entry_elems"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Insert an item with known value.
    grovedb::Bytes value{'h', 'e', 'l', 'l', 'o'};
    grovedb::Element item;
    BOOST_REQUIRE(grovedb::Element::Item(value, item).ok());
    BOOST_REQUIRE(db.Put(root, {'k'}, item, cost).ok());

    // Also retrieve the element directly for comparison.
    grovedb::Element fetched;
    BOOST_REQUIRE(db.GetDirect(root, {'k'}, fetched, cost).ok());

    // Prove and verify.
    grovedb::PathQuery query;
    BOOST_REQUIRE(grovedb::PathQuery::New(
        root,
        {grovedb::QueryItem::Key({'k'})},
        0, 0,
        query).ok());

    grovedb::Bytes proof;
    BOOST_REQUIRE(db.Prove(query, proof, cost).ok());

    grovedb::Hash root_hash{};
    std::vector<grovedb::ProofResultEntry> entries;
    BOOST_REQUIRE(grovedb::Db::VerifyQuery(proof, query, root_hash, entries).ok());
    BOOST_REQUIRE_EQUAL(entries.size(), 1);
    BOOST_REQUIRE(entries[0].m_element.has_value());

    // The serialized element from proof verification should match
    // what we get from direct retrieval.
    BOOST_CHECK(entries[0].m_element->data() == fetched.data());
}

BOOST_AUTO_TEST_SUITE_END()

// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// FUZZ TARGET: fuzz_proof
// Demonstrates: Prove(), VerifyQuery(), GetRootHash(),
//               ProveOptions, VerifyOptions
//

#include <FuzzedDataProvider.h>
#include <utils/check.h>
#include <utils/grovedb.h>
#include <utils/tempdir.h>

#include <grovedb/db.h>

#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    grovedb::fuzz::TempDir tmp{"fuzz_proof"};
    grovedb::Db db;
    if (!grovedb::Db::Open(tmp.path(), db).ok()) return 0;

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Insert a few items at root.
    auto n = fdp.ConsumeIntegralInRange<size_t>(1, 8);
    for (size_t i{0}; i < n && fdp.remaining_bytes() > 0; ++i) {
        grovedb::Bytes key{static_cast<uint8_t>(i)};
        grovedb::Bytes val = grovedb::fuzz::ConsumeValue(fdp);
        grovedb::Element item;
        if (!grovedb::Element::Item(val, item).ok()) continue;
        (void)db.Put(root, key, item, cost);
    }

    // Build a RangeFull query.
    grovedb::PathQuery pq;
    auto s = grovedb::PathQuery::New(
        root, {grovedb::QueryItem::RangeFull()},
        /*limit=*/0, /*offset=*/0, pq);
    if (!s.ok()) return 0;

    // Generate proof.
    grovedb::Bytes proof;
    s = db.Prove(pq, proof, cost);
    if (!s.ok()) return 0;

    // Build an identical query for verification (PathQuery is move-only).
    grovedb::PathQuery pq_verify;
    s = grovedb::PathQuery::New(
        root, {grovedb::QueryItem::RangeFull()},
        /*limit=*/0, /*offset=*/0, pq_verify);
    if (!s.ok()) return 0;

    // Verify the proof.
    grovedb::Hash proof_hash{};
    std::vector<grovedb::ProofResultEntry> entries;
    s = grovedb::Db::VerifyQuery(proof, pq_verify, proof_hash, entries);
    if (!s.ok()) return 0;

    // Root hash from the proof should match GetRootHash().
    grovedb::Hash db_hash{};
    CHECK_OK(db.GetRootHash(db_hash, cost));
    CHECK_EQ(proof_hash, db_hash);

    // Optionally flip a byte in the proof and verify it fails.
    if (!proof.empty() && fdp.remaining_bytes() > 0) {
        auto idx = fdp.ConsumeIntegralInRange<size_t>(0, proof.size() - 1);
        proof[idx] ^= 0xff;

        grovedb::PathQuery pq_bad;
        s = grovedb::PathQuery::New(
            root, {grovedb::QueryItem::RangeFull()},
            /*limit=*/0, /*offset=*/0, pq_bad);
        if (s.ok()) {
            grovedb::Hash bad_hash{};
            std::vector<grovedb::ProofResultEntry> bad_entries;
            // This may or may not fail -- the important thing is no crash.
            (void)grovedb::Db::VerifyQuery(proof, pq_bad, bad_hash, bad_entries);
        }
    }

    return 0;
}

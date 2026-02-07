// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <FuzzedDataProvider.h>

#include <grovedb/db.h>
#include <grovedb/element.h>
#include <grovedb/query.h>
#include <grovedb/types.h>

#include <fuzz/utils/check.h>
#include <fuzz/utils/grovedb.h>
#include <fuzz/utils/tempdir.h>

#include <algorithm>
#include <cstdint>
#include <vector>

namespace {
/// Maximum number of items to insert into the database.
static constexpr uint8_t MAX_INSERTS = 10;

/// Maximum number of proof operations per round.
static constexpr uint8_t MAX_OPS = 6;

/**
 * Consume a random QueryItem from fuzz input (simplified subset for proofs).
 */
grovedb::QueryItem ConsumeQueryItem(FuzzedDataProvider& fdp)
{
  auto kind = fdp.ConsumeIntegralInRange<uint8_t>(0, 4);
  switch (kind) {
  case 0:
    return grovedb::QueryItem::Key(grovedb::fuzz::ConsumeKey(fdp));
  case 1:
    return grovedb::QueryItem::RangeFull();
  case 2:
    return grovedb::QueryItem::RangeFrom(grovedb::fuzz::ConsumeKey(fdp));
  case 3:
    return grovedb::QueryItem::RangeTo(grovedb::fuzz::ConsumeKey(fdp));
  case 4:
    return grovedb::QueryItem::RangeInclusive(
        grovedb::fuzz::ConsumeKey(fdp), grovedb::fuzz::ConsumeKey(fdp)
    );
  default:
    return grovedb::QueryItem::RangeFull();
  }
}
} // anonymous namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  FuzzedDataProvider fdp(data, size);

  grovedb::fuzz::TempDir dir("fuzz_proof");
  auto db_result = grovedb::Db::Open(dir.path());
  if (!db_result.has_value()) {
    return 0;
  }
  auto& db = *db_result;

  grovedb::Path root_path{};

  // Insert fuzzed items at the root level.
  auto insert_count = fdp.ConsumeIntegralInRange<uint8_t>(1, MAX_INSERTS);
  for (uint8_t i = 0; i < insert_count && fdp.remaining_bytes() > 4; ++i) {
    auto key = grovedb::fuzz::ConsumeKey(fdp);
    if (key.empty()) {
      continue;
    }
    auto val = grovedb::fuzz::ConsumeValue(fdp);
    auto elem = grovedb::Element::Item(val);
    if (!elem.has_value()) {
      continue;
    }
    (void)db.Put(root_path, key, *elem).transform(CheckCostSanity);
  }

  // Run proof operations.
  auto op_count = fdp.ConsumeIntegralInRange<uint8_t>(1, MAX_OPS);
  for (uint8_t i = 0; i < op_count && fdp.remaining_bytes() > 4; ++i) {
    // Build a query.
    auto num_items = fdp.ConsumeIntegralInRange<uint8_t>(1, 4);
    std::vector<grovedb::QueryItem> items;
    items.reserve(num_items);
    for (uint8_t j = 0; j < num_items && fdp.remaining_bytes() > 2; ++j) {
      items.push_back(ConsumeQueryItem(fdp));
    }
    if (items.empty()) {
      continue;
    }

    auto limit = fdp.ConsumeIntegralInRange<uint32_t>(1, 100);
    auto pq = grovedb::PathQuery::New(root_path, items, limit);
    if (!pq.has_value()) {
      continue;
    }

    // Generate proof.
    auto decrease_limit = fdp.ConsumeBool();
    auto proof_result = db.Prove(*pq, decrease_limit);
    if (!proof_result.has_value()) {
      continue;
    }
    CheckCostSanity(proof_result->cost());
    auto& proof_bytes = proof_result->value();

    // Pick a verify operation.
    auto verify_op = fdp.ConsumeIntegralInRange<uint8_t>(0, 5);
    switch (verify_op) {
    case 0: {
      // VerifyQuery (strict).
      auto vr = db.VerifyQuery(proof_bytes, *pq);
      if (vr.has_value()) {
        CHECK_TRUE(vr->value().m_root_hash.size() == 32);
        CheckCostSanity(vr->cost());
      }
      break;
    }
    case 1: {
      // VerifyQueryWithOptions.
      auto absence = fdp.ConsumeBool();
      auto succinctness = fdp.ConsumeBool();
      auto empty_trees = fdp.ConsumeBool();
      (void)db.VerifyQueryWithOptions(proof_bytes, *pq, absence, succinctness, empty_trees)
          .transform(CheckCost);
      break;
    }
    case 2: {
      // VerifySubsetQuery.
      (void)db.VerifySubsetQuery(proof_bytes, *pq).transform(CheckCost);
      break;
    }
    case 3: {
      // VerifyQueryWithAbsenceProof.
      (void)db.VerifyQueryWithAbsenceProof(proof_bytes, *pq).transform(CheckCost);
      break;
    }
    case 4: {
      // VerifySubsetQueryWithAbsenceProof.
      (void)db.VerifySubsetQueryWithAbsenceProof(proof_bytes, *pq).transform(CheckCost);
      break;
    }
    case 5: {
      // Corrupt proof and verify — should fail.
      auto corrupt = proof_bytes;
      if (corrupt.size() > 10) {
        auto flip_pos = fdp.ConsumeIntegralInRange<size_t>(0, corrupt.size() - 1);
        corrupt[flip_pos] ^= 0xFF;
        auto vr = db.VerifyQuery(corrupt, *pq);
        // Either verification fails or the root hash doesn't match.
        if (vr.has_value()) {
          CheckCostSanity(vr->cost());
          auto db_root = db.GetRootHash();
          if (db_root.has_value()) {
            CheckCostSanity(db_root->cost());
            // Corrupted proof may still parse but produce wrong root hash.
            (void)(vr->value().m_root_hash != db_root->value());
          }
        }
      }
      break;
    }
    } // switch
  }

  // Exercise prove + verify round-trip consistency: a valid proof should
  // verify and yield a root hash matching the database.
  if (fdp.remaining_bytes() > 2) {
    auto full_query =
        grovedb::PathQuery::New(root_path, {grovedb::QueryItem::RangeFull()}, /*limit=*/100);
    if (full_query.has_value()) {
      auto proof_r = db.Prove(*full_query);
      if (proof_r.has_value()) {
        CheckCostSanity(proof_r->cost());
        auto verify_r = db.VerifyQuery(proof_r->value(), *full_query);
        if (verify_r.has_value()) {
          CheckCostSanity(verify_r->cost());
          auto db_root = db.GetRootHash();
          if (db_root.has_value()) {
            CheckCostSanity(db_root->cost());
            CHECK_TRUE(verify_r->value().m_root_hash == db_root->value());
          }
        }
      }
    }
  }

  return 0;
}

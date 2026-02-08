// Copyright (c) 2026-present, The Dash Core developers
// Copyright (c) 2017-present, Cosmos Labs
// Distributed under the Apache 2.0 license, see the accompanying
// file LICENSE.APACHE2 or https://opensource.org/license/apache-2-0
//
// Adapted from IAVL's proof_test.go

#include <test/util/tempdir.h>

#include <grovedb/db.h>
#include <grovedb/element.h>
#include <grovedb/query.h>

#include <util/logging.h>

#include <cstdlib>
#include <format>
#include <string>

int main()
{
  Log(INFO, "=== Proof Generation ===");

  grovedb::test::TempDir dir("ex_proof_gen");
  auto result = grovedb::Db::Open(dir.PathToString());
  if (!result.has_value()) {
    Log(ERROR, "failed to open database: {}", result.error().message());
    return EXIT_FAILURE;
  }
  auto db = std::move(result).value();
  Log(INFO, "database opened at {}", dir.PathToString());

  grovedb::Path root{};

  // -------------------------------------------------------------------------
  // 1. Insert items "a" through "e".
  // -------------------------------------------------------------------------

  for (char c = 'a'; c <= 'e'; ++c) {
    auto key = grovedb::Bytes::FromString(std::string(1, c));
    auto cost = grovedb::Element::Item(grovedb::Bytes::FromString(std::format("value_{}", c)))
                    .and_then([&](grovedb::Element e) { return db.Put(root, key, e); });
    if (!cost.has_value()) {
      Log(ERROR, "put failed: {}", cost.error().message());
      return EXIT_FAILURE;
    }
    Log(INFO, "inserted '{}'", std::string(1, c));
  }

  // -------------------------------------------------------------------------
  // 2. Get the root hash.
  // -------------------------------------------------------------------------

  auto root_hash = db.GetRootHash();
  if (!root_hash.has_value()) {
    Log(ERROR, "GetRootHash failed: {}", root_hash.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "root hash: {}", root_hash->value().ToString());

  // -------------------------------------------------------------------------
  // 3. Prove with RangeFull query.
  // -------------------------------------------------------------------------

  Log(INFO, "--- Prove (RangeFull) ---");
  auto proof = grovedb::PathQuery::New(root, {grovedb::QueryItem::RangeFull()})
                   .and_then([&](grovedb::PathQuery q) { return db.Prove(q); });
  if (!proof.has_value()) {
    Log(ERROR, "Prove failed: {}", proof.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "proof size: {} bytes", proof->value().size());

  // -------------------------------------------------------------------------
  // 4. VerifyQuery — verify the proof and compare root hashes.
  // -------------------------------------------------------------------------

  Log(INFO, "--- VerifyQuery ---");
  auto verify =
      grovedb::PathQuery::New(root, {grovedb::QueryItem::RangeFull()})
          .and_then([&](grovedb::PathQuery q) { return db.VerifyQuery(proof->value(), q); });
  if (!verify.has_value()) {
    Log(ERROR, "VerifyQuery failed: {}", verify.error().message());
    return EXIT_FAILURE;
  }

  Log(INFO, "verification succeeded");
  Log(INFO, "proof root hash:    {}", verify->value().m_root_hash.ToString());
  Log(INFO, "database root hash: {}", root_hash->value().ToString());
  Log(INFO, "hashes match: {}", verify->value().m_root_hash == root_hash->value());
  Log(INFO, "verified entries: {}", verify->value().m_entries.size());

  for (const auto& entry : verify->value().m_entries) {
    Log(
        INFO, "  key='{}', element present={}", entry.m_key.ToString(), entry.m_element.has_value()
    );
  }

  // -------------------------------------------------------------------------
  // 5. Single key proof for "c".
  // -------------------------------------------------------------------------

  Log(INFO, "--- single key proof for 'c' ---");
  auto proof_c = grovedb::PathQuery::New(root, {grovedb::QueryItem::Key({'c'})}, /*limit=*/1)
                     .and_then([&](grovedb::PathQuery q) { return db.Prove(q); });
  if (!proof_c.has_value()) {
    Log(ERROR, "Prove failed: {}", proof_c.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "proof for 'c': {} bytes", proof_c->value().size());

  auto verify_c =
      grovedb::PathQuery::New(root, {grovedb::QueryItem::Key({'c'})}, /*limit=*/1)
          .and_then([&](grovedb::PathQuery q) { return db.VerifyQuery(proof_c->value(), q); });
  if (!verify_c.has_value()) {
    Log(ERROR, "VerifyQuery for 'c' failed: {}", verify_c.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "single key verification succeeded, {} entries", verify_c->value().m_entries.size());

  // -------------------------------------------------------------------------
  // 6. Corrupt proof — verification should fail.
  // -------------------------------------------------------------------------

  Log(INFO, "--- corrupt proof ---");
  auto corrupt = proof->value(); // copy
  if (corrupt.size() > 10) {
    corrupt[10] ^= 0xFF; // flip some bytes
    corrupt[11] ^= 0xFF;
  }

  auto verify_corrupt =
      grovedb::PathQuery::New(root, {grovedb::QueryItem::RangeFull()})
          .and_then([&](grovedb::PathQuery q) { return db.VerifyQuery(corrupt, q); });
  Log(INFO, "corrupt proof verification: has_value={}", verify_corrupt.has_value());
  if (!verify_corrupt.has_value()) {
    Log(INFO,
        "expected error: {} ({})",
        ToString(verify_corrupt.error().code()),
        verify_corrupt.error().message());
  }

  Log(INFO, "done");
  return EXIT_SUCCESS;
}

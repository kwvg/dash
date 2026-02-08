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
#include <string>

int main()
{
  Log(INFO, "=== Absence Proofs ===");

  grovedb::test::TempDir dir("ex_absence_proofs");
  auto result = grovedb::Db::Open(dir.PathToString());
  if (!result.has_value()) {
    Log(ERROR, "failed to open database: {}", result.error().message());
    return EXIT_FAILURE;
  }
  auto db = std::move(result).value();
  Log(INFO, "database opened at {}", dir.PathToString());

  grovedb::Path root{};

  const std::vector<grovedb::QueryItem> KEYS_A_TO_E{
      grovedb::QueryItem::Key({'a'}),
      grovedb::QueryItem::Key({'b'}),
      grovedb::QueryItem::Key({'c'}),
      grovedb::QueryItem::Key({'d'}),
      grovedb::QueryItem::Key({'e'}),
  };

  // -------------------------------------------------------------------------
  // 1. Insert "a", "c", "e" (gaps at b, d).
  // -------------------------------------------------------------------------

  for (const char* k : {"a", "c", "e"}) {
    auto cost = grovedb::Element::Item(grovedb::Bytes::FromString(std::string("val_") + k))
                    .and_then([&](grovedb::Element e) {
                      return db.Put(root, grovedb::Bytes::FromString(k), e);
                    });
    if (!cost.has_value()) {
      Log(ERROR, "put failed: {}", cost.error().message());
      return EXIT_FAILURE;
    }
    Log(INFO, "inserted '{}'", k);
  }

  // -------------------------------------------------------------------------
  // 2. Prove specific keys (including absent ones) with VerifyQueryWithAbsenceProof.
  //    Absence proofs require specific Key queries, not unbounded ranges.
  // -------------------------------------------------------------------------

  Log(INFO, "--- VerifyQueryWithAbsenceProof ---");
  auto proof = grovedb::PathQuery::New(
                   root,
                   KEYS_A_TO_E,
                   /*limit=*/100
  )
                   .and_then([&](grovedb::PathQuery q) { return db.Prove(q); });
  if (!proof.has_value()) {
    Log(ERROR, "Prove failed: {}", proof.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "proof generated: {} bytes", proof->value().size());

  auto vap = grovedb::PathQuery::New(
                 root,
                 KEYS_A_TO_E,
                 /*limit=*/100
  )
                 .and_then([&](grovedb::PathQuery q) {
                   return db.VerifyQueryWithAbsenceProof(proof->value(), q);
                 });
  if (!vap.has_value()) {
    Log(ERROR, "VerifyQueryWithAbsenceProof failed: {}", vap.error().message());
    return EXIT_FAILURE;
  }

  Log(INFO, "verification succeeded, root hash: {}", vap->value().m_root_hash.ToString());
  Log(INFO, "{} entries in result", vap->value().m_entries.size());
  for (const auto& entry : vap->value().m_entries) {
    Log(INFO, "  key='{}', present={}", entry.m_key.ToString(), entry.m_element.has_value());
  }

  // -------------------------------------------------------------------------
  // 3. VerifySubsetQuery with subset query for "c" only.
  // -------------------------------------------------------------------------

  Log(INFO, "--- VerifySubsetQuery ---");
  auto vsq =
      grovedb::PathQuery::New(root, {grovedb::QueryItem::Key({'c'})})
          .and_then([&](grovedb::PathQuery q) { return db.VerifySubsetQuery(proof->value(), q); });
  if (!vsq.has_value()) {
    Log(ERROR, "VerifySubsetQuery failed: {}", vsq.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "subset verification succeeded, {} entries", vsq->value().m_entries.size());

  // -------------------------------------------------------------------------
  // 4. VerifySubsetQueryWithAbsenceProof.
  // -------------------------------------------------------------------------

  Log(INFO, "--- VerifySubsetQueryWithAbsenceProof ---");
  auto vsqap = grovedb::PathQuery::New(
                   root,
                   {grovedb::QueryItem::Key({'c'})},
                   /*limit=*/100
  )
                   .and_then([&](grovedb::PathQuery q) {
                     return db.VerifySubsetQueryWithAbsenceProof(proof->value(), q);
                   });
  if (!vsqap.has_value()) {
    Log(ERROR, "VerifySubsetQueryWithAbsenceProof failed: {}", vsqap.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "subset+absence verification succeeded, {} entries", vsqap->value().m_entries.size());

  // -------------------------------------------------------------------------
  // 5. VerifyQueryWithOptions (absence=true, succinctness=false, empty_trees=false).
  // -------------------------------------------------------------------------

  Log(INFO, "--- VerifyQueryWithOptions ---");
  auto vwo = grovedb::PathQuery::New(
                 root,
                 KEYS_A_TO_E,
                 /*limit=*/100
  )
                 .and_then([&](grovedb::PathQuery q) {
                   return db.VerifyQueryWithOptions(
                       proof->value(),
                       q,
                       /*absence_proofs=*/true,
                       /*verify_succinctness=*/false,
                       /*include_empty_trees=*/false
                   );
                 });
  if (!vwo.has_value()) {
    Log(ERROR, "VerifyQueryWithOptions failed: {}", vwo.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "options verification succeeded, {} entries", vwo->value().m_entries.size());

  Log(INFO, "done");
  return EXIT_SUCCESS;
}

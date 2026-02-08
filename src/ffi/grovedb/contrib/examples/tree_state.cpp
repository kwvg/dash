// Copyright (c) 2026-present, The Dash Core developers
// Copyright (c) 2017-present, Cosmos Labs
// Distributed under the Apache 2.0 license, see the accompanying
// file LICENSE.APACHE2 or https://opensource.org/license/apache-2-0
//
// Adapted from IAVL's tree_test.go

#include <test/util/tempdir.h>

#include <grovedb/db.h>
#include <grovedb/element.h>

#include <util/logging.h>

#include <cstdlib>
#include <format>
#include <string>

int main()
{
  Log(INFO, "=== Tree State ===");

  grovedb::test::TempDir dir("ex_tree_state");
  auto result = grovedb::Db::Open(dir.PathToString());
  if (!result.has_value()) {
    Log(ERROR, "failed to open database: {}", result.error().message());
    return EXIT_FAILURE;
  }
  auto db = std::move(result).value();
  Log(INFO, "database opened at {}", dir.PathToString());

  grovedb::Path root{};

  // -------------------------------------------------------------------------
  // 1. Root hash of empty tree.
  // -------------------------------------------------------------------------

  Log(INFO, "--- empty tree root hash ---");
  auto hash_empty = db.GetRootHash();
  if (!hash_empty.has_value()) {
    Log(ERROR, "GetRootHash failed: {}", hash_empty.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "root hash (empty): {}", hash_empty->value().ToString());

  // -------------------------------------------------------------------------
  // 2. Insert items and observe root hash change.
  // -------------------------------------------------------------------------

  Log(INFO, "--- inserting items ---");
  auto cost_tree = grovedb::Element::EmptyTree().and_then([&](grovedb::Element e) {
    return db.Put(root, grovedb::Bytes::FromString("subtree"), e);
  });
  if (!cost_tree.has_value()) {
    Log(ERROR, "put subtree failed: {}", cost_tree.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "created subtree at root/'subtree'");

  grovedb::Path subtree{grovedb::Bytes::FromString("subtree")};

  for (int i = 0; i < 3; ++i) {
    auto key = grovedb::Bytes::FromString(std::format("key_{}", i));
    auto c = grovedb::Element::Item(grovedb::Bytes::FromString(std::format("val_{}", i)))
                 .and_then([&](grovedb::Element e) { return db.Put(subtree, key, e); });
    if (!c.has_value()) {
      Log(ERROR, "put failed: {}", c.error().message());
      return EXIT_FAILURE;
    }
    Log(INFO, "inserted key_{} into subtree", i);
  }

  auto hash_after = db.GetRootHash();
  if (!hash_after.has_value()) {
    Log(ERROR, "GetRootHash failed: {}", hash_after.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "root hash (after inserts): {}", hash_after->value().ToString());
  Log(INFO, "root hash changed: {}", hash_empty->value() != hash_after->value());

  // -------------------------------------------------------------------------
  // 3. Verify integrity.
  // -------------------------------------------------------------------------

  Log(INFO, "--- integrity check ---");
  auto integrity = db.VerifyIntegrity();
  if (!integrity.has_value()) {
    Log(ERROR, "VerifyIntegrity failed: {}", integrity.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "VerifyIntegrity() = {}", integrity.value());

  // -------------------------------------------------------------------------
  // 4. Delete a key.
  // -------------------------------------------------------------------------

  Log(INFO, "--- delete key_1 ---");
  auto del = db.Delete(subtree, grovedb::Bytes::FromString("key_1"));
  if (!del.has_value()) {
    Log(ERROR, "delete failed: {}", del.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "deleted key_1: {} seeks", del->m_seek_count);

  // -------------------------------------------------------------------------
  // 5. DeleteIfEmpty — non-empty subtree returns false.
  // -------------------------------------------------------------------------

  Log(INFO, "--- DeleteIfEmpty on non-empty subtree ---");
  auto die1 = db.DeleteIfEmpty(root, grovedb::Bytes::FromString("subtree"));
  if (!die1.has_value()) {
    Log(ERROR, "DeleteIfEmpty failed: {}", die1.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "DeleteIfEmpty(root, 'subtree') = {} (subtree has items)", die1->value());

  // Delete remaining items to make subtree empty, then DeleteIfEmpty.
  Log(INFO, "deleting remaining items from subtree...");
  for (int i : {0, 2}) {
    auto c = db.Delete(subtree, grovedb::Bytes::FromString(std::format("key_{}", i)));
    if (!c.has_value()) {
      Log(ERROR, "delete failed: {}", c.error().message());
      return EXIT_FAILURE;
    }
    Log(INFO, "deleted key_{}", i);
  }

  auto die2 = db.DeleteIfEmpty(root, grovedb::Bytes::FromString("subtree"));
  if (!die2.has_value()) {
    Log(ERROR, "DeleteIfEmpty failed: {}", die2.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "DeleteIfEmpty(root, 'subtree') = {} (subtree is now empty)", die2->value());

  // -------------------------------------------------------------------------
  // 6. PruneEmptyAncestors on a deep chain.
  // -------------------------------------------------------------------------

  Log(INFO, "--- PruneEmptyAncestors ---");

  // Build: root -> "a" -> "b" -> item "leaf"
  auto ca = grovedb::Element::EmptyTree().and_then([&](grovedb::Element e) {
    return db.Put(root, {'a'}, e);
  });
  if (!ca.has_value()) {
    Log(ERROR, "put failed: {}", ca.error().message());
    return EXIT_FAILURE;
  }

  grovedb::Path path_a{{'a'}};
  auto cb = grovedb::Element::EmptyTree().and_then([&](grovedb::Element e) {
    return db.Put(path_a, {'b'}, e);
  });
  if (!cb.has_value()) {
    Log(ERROR, "put failed: {}", cb.error().message());
    return EXIT_FAILURE;
  }

  grovedb::Path path_ab{{'a'}, {'b'}};
  auto cl = grovedb::Element::Item(grovedb::Bytes::FromString("leaf_data"))
                .and_then([&](grovedb::Element e) {
                  return db.Put(path_ab, grovedb::Bytes::FromString("leaf"), e);
                });
  if (!cl.has_value()) {
    Log(ERROR, "put failed: {}", cl.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "created root/a/b/leaf");

  auto prune = db.PruneEmptyAncestors(path_ab, grovedb::Bytes::FromString("leaf"));
  if (!prune.has_value()) {
    Log(ERROR, "PruneEmptyAncestors failed: {}", prune.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "PruneEmptyAncestors removed {} level(s)", prune->value());

  // -------------------------------------------------------------------------
  // 7. Clear a subtree.
  // -------------------------------------------------------------------------

  Log(INFO, "--- Clear subtree ---");

  // Create a new subtree with items.
  auto ct = grovedb::Element::EmptyTree().and_then([&](grovedb::Element e) {
    return db.Put(root, grovedb::Bytes::FromString("clearme"), e);
  });
  if (!ct.has_value()) {
    Log(ERROR, "put failed: {}", ct.error().message());
    return EXIT_FAILURE;
  }

  grovedb::Path clear_path{grovedb::Bytes::FromString("clearme")};
  for (int i = 0; i < 3; ++i) {
    auto c = grovedb::Element::Item(grovedb::Bytes::FromString(std::format("v{}", i)))
                 .and_then([&](grovedb::Element e) {
                   return db.Put(clear_path, grovedb::Bytes::FromString(std::format("k{}", i)), e);
                 });
    if (!c.has_value()) {
      Log(ERROR, "put failed: {}", c.error().message());
      return EXIT_FAILURE;
    }
  }

  auto before = db.IsEmptyTree(clear_path);
  if (!before.has_value()) {
    Log(ERROR, "IsEmptyTree failed: {}", before.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "IsEmptyTree('clearme') before Clear: {}", before->value());

  auto clear = db.Clear(clear_path);
  if (!clear.has_value()) {
    Log(ERROR, "Clear failed: {}", clear.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "Clear('clearme') succeeded");

  auto after = db.IsEmptyTree(clear_path);
  if (!after.has_value()) {
    Log(ERROR, "IsEmptyTree failed: {}", after.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "IsEmptyTree('clearme') after Clear: {}", after->value());

  // -------------------------------------------------------------------------
  // 8. Final root hash and integrity check.
  // -------------------------------------------------------------------------

  auto hash_final = db.GetRootHash();
  if (!hash_final.has_value()) {
    Log(ERROR, "GetRootHash failed: {}", hash_final.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "final root hash: {}", hash_final->value().ToString());

  auto final_integrity = db.VerifyIntegrity();
  if (!final_integrity.has_value()) {
    Log(ERROR, "VerifyIntegrity failed: {}", final_integrity.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "final VerifyIntegrity() = {}", final_integrity.value());

  Log(INFO, "done");
  return EXIT_SUCCESS;
}

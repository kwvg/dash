// Copyright (c) 2026-present, The Dash Core developers
// Copyright (c) 2011-present, Facebook, Inc.
// Distributed under the Apache 2.0 license, see the accompanying
// file LICENSE.APACHE2 or https://opensource.org/license/apache-2-0
//
// Adapted from RocksDb's example/simple_example.cc

#include <test/util/tempdir.h>

#include <grovedb/batch.h>
#include <grovedb/db.h>
#include <grovedb/element.h>

#include <util/logging.h>

#include <cstdlib>
#include <string>
#include <vector>

int main()
{
  Log(INFO, "=== Batch Operations ===");

  grovedb::test::TempDir dir("ex_batch_ops");
  auto result = grovedb::Db::Open(dir.PathToString());
  if (!result.has_value()) {
    Log(ERROR, "failed to open database: {}", result.error().message());
    return EXIT_FAILURE;
  }
  auto db = std::move(result).value();
  Log(INFO, "database opened at {}", dir.PathToString());

  grovedb::Path root{};

  // -------------------------------------------------------------------------
  // 1. Pre-insert keys so we can Replace and Delete them.
  // -------------------------------------------------------------------------

  auto pre = grovedb::Element::Item(grovedb::Bytes::FromString("old_value"))
                 .and_then([&](grovedb::Element e) {
                   return db.Put(root, grovedb::Bytes::FromString("replace_me"), e);
                 });
  if (!pre.has_value()) {
    Log(ERROR, "pre-insert failed: {}", pre.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "pre-inserted 'replace_me' = 'old_value'");

  auto pre2 = grovedb::Element::Item(grovedb::Bytes::FromString("doomed"))
                  .and_then([&](grovedb::Element e) {
                    return db.Put(root, grovedb::Bytes::FromString("delete_me"), e);
                  });
  if (!pre2.has_value()) {
    Log(ERROR, "pre-insert failed: {}", pre2.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "pre-inserted 'delete_me' = 'doomed'");

  // Also create a subtree so we can DeleteTree it.
  auto pt = grovedb::Element::EmptyTree().and_then([&](grovedb::Element e) {
    return db.Put(root, grovedb::Bytes::FromString("my_tree"), e);
  });
  if (!pt.has_value()) {
    Log(ERROR, "put subtree failed: {}", pt.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "pre-inserted subtree 'my_tree'");

  // -------------------------------------------------------------------------
  // 2. Build batch operations — each key used exactly once.
  // -------------------------------------------------------------------------

  Log(INFO, "--- building batch ---");

  auto new_elem = grovedb::Element::Item(grovedb::Bytes::FromString("brand_new"));
  if (!new_elem.has_value()) {
    Log(ERROR, "failed to create element: {}", new_elem.error().message());
    return EXIT_FAILURE;
  }

  auto replace_elem = grovedb::Element::Item(grovedb::Bytes::FromString("new_value"));
  if (!replace_elem.has_value()) {
    Log(ERROR, "failed to create element: {}", replace_elem.error().message());
    return EXIT_FAILURE;
  }

  auto upsert_elem = grovedb::Element::Item(grovedb::Bytes::FromString("upserted"));
  if (!upsert_elem.has_value()) {
    Log(ERROR, "failed to create element: {}", upsert_elem.error().message());
    return EXIT_FAILURE;
  }

  std::vector<grovedb::BatchOperation> ops{
      grovedb::BatchOperation::InsertOnly(root, grovedb::Bytes::FromString("new_key"), *new_elem),
      grovedb::BatchOperation::InsertOrReplace(
          root, grovedb::Bytes::FromString("upsert_key"), *upsert_elem
      ),
      grovedb::BatchOperation::Replace(
          root, grovedb::Bytes::FromString("replace_me"), *replace_elem
      ),
      grovedb::BatchOperation::Delete(root, grovedb::Bytes::FromString("delete_me")),
      grovedb::BatchOperation::DeleteTree(root, grovedb::Bytes::FromString("my_tree")),
  };

  Log(INFO,
      "built {} operations: InsertOnly, InsertOrReplace, Replace, Delete, DeleteTree",
      ops.size());

  // -------------------------------------------------------------------------
  // 3. Apply the batch atomically.
  // -------------------------------------------------------------------------

  grovedb::BatchApplyOptions options;
  options.m_allow_deleting_non_empty_trees = true;

  auto batch = db.ApplyBatch(ops, options);
  if (!batch.has_value()) {
    Log(ERROR, "ApplyBatch failed: {}", batch.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO,
      "batch applied atomically: {} seeks, {} bytes added",
      batch->m_seek_count,
      batch->m_storage_added_bytes);

  // -------------------------------------------------------------------------
  // 4. Verify each effect.
  // -------------------------------------------------------------------------

  Log(INFO, "--- verifying results ---");

  auto k1 = db.KeyExists(root, grovedb::Bytes::FromString("new_key"));
  Log(INFO, "KeyExists('new_key') = {}", k1.has_value() ? k1->value() : false);

  auto k2 = db.KeyExists(root, grovedb::Bytes::FromString("upsert_key"));
  Log(INFO, "KeyExists('upsert_key') = {}", k2.has_value() ? k2->value() : false);

  auto k3 = db.KeyExists(root, grovedb::Bytes::FromString("replace_me"));
  Log(INFO, "KeyExists('replace_me') = {} (was replaced)", k3.has_value() ? k3->value() : false);

  auto k4 = db.KeyExists(root, grovedb::Bytes::FromString("delete_me"));
  Log(INFO, "KeyExists('delete_me') = {} (was deleted)", k4.has_value() ? k4->value() : false);

  auto k5 = db.KeyExists(root, grovedb::Bytes::FromString("my_tree"));
  Log(INFO, "KeyExists('my_tree') = {} (was deleted)", k5.has_value() ? k5->value() : false);

  Log(INFO, "done");
  return EXIT_SUCCESS;
}

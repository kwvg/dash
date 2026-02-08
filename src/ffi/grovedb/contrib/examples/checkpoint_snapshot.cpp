// Copyright (c) 2026-present, The Dash Core developers
// Copyright (c) 2011-present, Facebook, Inc.
// Distributed under the Apache 2.0 license, see the accompanying
// file LICENSE.APACHE2 or https://opensource.org/license/apache-2-0
//
// Adapted from RocksDb's example/rocksdb_backup_restore_example.cc

#include <test/util/tempdir.h>

#include <grovedb/db.h>
#include <grovedb/element.h>

#include <util/logging.h>

#include <cstdlib>
#include <string>

int main()
{
  Log(INFO, "=== Checkpoint Snapshot ===");

  grovedb::test::TempDir dir("ex_checkpoint");
  // Open DB in a subdirectory of TempDir.
  std::string db_path = dir.PathToString() + "/db";
  auto result = grovedb::Db::Open(db_path);
  if (!result.has_value()) {
    Log(ERROR, "failed to open database: {}", result.error().message());
    return EXIT_FAILURE;
  }
  auto db = std::move(result).value();
  Log(INFO, "database opened at {}", db_path);

  grovedb::Path root{};

  // -------------------------------------------------------------------------
  // 1. Insert data before checkpoint.
  // -------------------------------------------------------------------------

  auto p1 = grovedb::Element::Item(grovedb::Bytes::FromString("before_value"))
                .and_then([&](grovedb::Element e) {
                  return db.Put(root, grovedb::Bytes::FromString("before_ckpt"), e);
                });
  if (!p1.has_value()) {
    Log(ERROR, "put failed: {}", p1.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "inserted 'before_ckpt'");

  // -------------------------------------------------------------------------
  // 2. Create checkpoint.
  // -------------------------------------------------------------------------

  // Checkpoint target must NOT exist yet.
  std::string ckpt_path = dir.PathToString() + "/ckpt";
  auto create = db.CreateCheckpoint(ckpt_path);
  if (!create.has_value()) {
    Log(ERROR, "CreateCheckpoint failed: {}", create.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "checkpoint created at {}", ckpt_path);

  // -------------------------------------------------------------------------
  // 3. Insert data AFTER checkpoint in original DB.
  // -------------------------------------------------------------------------

  auto p2 = grovedb::Element::Item(grovedb::Bytes::FromString("after_value"))
                .and_then([&](grovedb::Element e) {
                  return db.Put(root, grovedb::Bytes::FromString("after_ckpt"), e);
                });
  if (!p2.has_value()) {
    Log(ERROR, "put failed: {}", p2.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "inserted 'after_ckpt' in original DB (after checkpoint)");

  // -------------------------------------------------------------------------
  // 4. Open checkpoint and verify isolation.
  // -------------------------------------------------------------------------

  Log(INFO, "--- opening checkpoint ---");
  auto ckpt_db = grovedb::Db::OpenCheckpoint(ckpt_path);
  if (!ckpt_db.has_value()) {
    Log(ERROR, "OpenCheckpoint failed: {}", ckpt_db.error().message());
    return EXIT_FAILURE;
  }

  auto k_before = ckpt_db->KeyExists(root, grovedb::Bytes::FromString("before_ckpt"));
  if (!k_before.has_value()) {
    Log(ERROR, "KeyExists failed: {}", k_before.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "checkpoint KeyExists('before_ckpt') = {} (was in snapshot)", k_before->value());

  auto k_after = ckpt_db->KeyExists(root, grovedb::Bytes::FromString("after_ckpt"));
  if (!k_after.has_value()) {
    Log(ERROR, "KeyExists failed: {}", k_after.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "checkpoint KeyExists('after_ckpt') = {} (was NOT in snapshot)", k_after->value());

  // -------------------------------------------------------------------------
  // 5. Compare root hashes — should differ.
  // -------------------------------------------------------------------------

  Log(INFO, "--- root hash comparison ---");
  auto db_hash = db.GetRootHash();
  if (!db_hash.has_value()) {
    Log(ERROR, "GetRootHash failed: {}", db_hash.error().message());
    return EXIT_FAILURE;
  }

  auto ckpt_hash = ckpt_db->GetRootHash();
  if (!ckpt_hash.has_value()) {
    Log(ERROR, "GetRootHash failed: {}", ckpt_hash.error().message());
    return EXIT_FAILURE;
  }

  Log(INFO, "original DB root hash:  {}", db_hash->value().ToString());
  Log(INFO, "checkpoint root hash:   {}", ckpt_hash->value().ToString());
  Log(INFO, "hashes differ: {}", db_hash->value() != ckpt_hash->value());

  // Close checkpoint handle before deleting by moving it into a temporary scope.
  {
    auto _ = std::move(*ckpt_db);
  }

  // -------------------------------------------------------------------------
  // 6. Delete checkpoint → verify open fails.
  // -------------------------------------------------------------------------

  Log(INFO, "--- delete checkpoint ---");
  auto del = grovedb::Db::DeleteCheckpoint(ckpt_path);
  if (!del.has_value()) {
    Log(ERROR, "DeleteCheckpoint failed: {}", del.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "checkpoint deleted");

  auto reopen = grovedb::Db::OpenCheckpoint(ckpt_path);
  Log(INFO, "OpenCheckpoint after delete: has_value={} (expected false)", reopen.has_value());
  if (!reopen.has_value()) {
    Log(INFO, "expected error: {}", reopen.error().message());
  }

  Log(INFO, "done");
  return EXIT_SUCCESS;
}

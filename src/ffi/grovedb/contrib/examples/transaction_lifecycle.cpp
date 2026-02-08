// Copyright (c) 2026-present, The Dash Core developers
// Copyright (c) 2011-present, Facebook, Inc.
// Distributed under the Apache 2.0 license, see the accompanying
// file LICENSE.APACHE2 or https://opensource.org/license/apache-2-0
//
// Adapted from RocksDb's example/transaction_example.cc

#include <test/util/tempdir.h>

#include <grovedb/db.h>
#include <grovedb/element.h>

#include <util/logging.h>

#include <cstdlib>
#include <string>

int main()
{
  Log(INFO, "=== Transaction Lifecycle ===");

  grovedb::test::TempDir dir("ex_transaction");
  auto result = grovedb::Db::Open(dir.PathToString());
  if (!result.has_value()) {
    Log(ERROR, "failed to open database: {}", result.error().message());
    return EXIT_FAILURE;
  }
  auto db = std::move(result).value();
  Log(INFO, "database opened at {}", dir.PathToString());

  grovedb::Path root{};

  // -------------------------------------------------------------------------
  // 1. Commit path: begin → put → read in txn → show isolation → commit.
  // -------------------------------------------------------------------------

  Log(INFO, "--- commit path ---");

  auto txn = db.BeginTransaction();
  if (!txn.has_value()) {
    Log(ERROR, "BeginTransaction failed: {}", txn.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "transaction started");

  auto elem = grovedb::Element::Item(grovedb::Bytes::FromString("txn_value"));
  if (!elem.has_value()) {
    Log(ERROR, "failed to create element: {}", elem.error().message());
    return EXIT_FAILURE;
  }

  auto put = db.Put(root, grovedb::Bytes::FromString("txn_key"), *elem, *txn);
  if (!put.has_value()) {
    Log(ERROR, "put in txn failed: {}", put.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "put 'txn_key' inside transaction: {} seeks", put->m_seek_count);

  // Read within the transaction — should be visible.
  auto get_in = db.Get(root, grovedb::Bytes::FromString("txn_key"), *txn);
  Log(INFO, "get 'txn_key' inside txn: has_value={}", get_in.has_value());

  // Read outside the transaction — should NOT be visible yet.
  auto get_out = db.Get(root, grovedb::Bytes::FromString("txn_key"));
  Log(INFO, "get 'txn_key' outside txn (isolation): has_value={}", get_out.has_value());

  // Commit.
  auto commit = db.Commit(*txn);
  if (!commit.has_value()) {
    Log(ERROR, "commit failed: {}", commit.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO,
      "transaction committed: {} seeks, {} bytes added",
      commit->m_seek_count,
      commit->m_storage_added_bytes);

  // Now visible outside.
  auto get_after = db.Get(root, grovedb::Bytes::FromString("txn_key"));
  Log(INFO, "get 'txn_key' after commit: has_value={}", get_after.has_value());

  // -------------------------------------------------------------------------
  // 2. Rollback path: begin → put → rollback → verify absent.
  // -------------------------------------------------------------------------

  Log(INFO, "--- rollback path ---");

  auto txn2 = db.BeginTransaction();
  if (!txn2.has_value()) {
    Log(ERROR, "BeginTransaction failed: {}", txn2.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "transaction started");

  auto elem2 = grovedb::Element::Item(grovedb::Bytes::FromString("rollback_val"));
  if (!elem2.has_value()) {
    Log(ERROR, "failed to create element: {}", elem2.error().message());
    return EXIT_FAILURE;
  }

  auto put2 = db.Put(root, grovedb::Bytes::FromString("rollback_key"), *elem2, *txn2);
  if (!put2.has_value()) {
    Log(ERROR, "put in txn failed: {}", put2.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "put 'rollback_key' inside transaction");

  auto rb = db.Rollback(*txn2);
  if (!rb.has_value()) {
    Log(ERROR, "rollback failed: {}", rb.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "transaction rolled back");

  auto get_rb = db.KeyExists(root, grovedb::Bytes::FromString("rollback_key"));
  if (!get_rb.has_value()) {
    Log(ERROR, "key exists check failed: {}", get_rb.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "key 'rollback_key' exists after rollback: {}", get_rb->value());

  // -------------------------------------------------------------------------
  // 3. RAII auto-rollback: transaction goes out of scope without commit.
  // -------------------------------------------------------------------------

  Log(INFO, "--- RAII auto-rollback ---");

  {
    auto txn3 = db.BeginTransaction();
    if (!txn3.has_value()) {
      Log(ERROR, "BeginTransaction failed: {}", txn3.error().message());
      return EXIT_FAILURE;
    }
    Log(INFO, "transaction started (will be destroyed without commit)");

    auto elem3 = grovedb::Element::Item(grovedb::Bytes::FromString("raii_val"));
    if (!elem3.has_value()) {
      Log(ERROR, "failed to create element: {}", elem3.error().message());
      return EXIT_FAILURE;
    }

    auto put3 = db.Put(root, grovedb::Bytes::FromString("raii_key"), *elem3, *txn3);
    if (!put3.has_value()) {
      Log(ERROR, "put in txn failed: {}", put3.error().message());
      return EXIT_FAILURE;
    }
    Log(INFO, "put 'raii_key' inside transaction");
    // txn3 destroyed here — auto-rollback.
  }

  auto get_raii = db.KeyExists(root, grovedb::Bytes::FromString("raii_key"));
  if (!get_raii.has_value()) {
    Log(ERROR, "key exists check failed: {}", get_raii.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO,
      "key 'raii_key' exists after RAII destruction: {} (auto-rolled back)",
      get_raii->value());

  Log(INFO, "done");
  return EXIT_SUCCESS;
}

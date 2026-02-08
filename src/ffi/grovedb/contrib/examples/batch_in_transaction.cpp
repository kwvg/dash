// Copyright (c) 2026-present, The Dash Core developers
// Copyright (c) 2011-present, Facebook, Inc.
// Distributed under the Apache 2.0 license, see the accompanying
// file LICENSE.APACHE2 or https://opensource.org/license/apache-2-0
//
// Adapted from RocksDb's example/transaction_example.cc

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
  Log(INFO, "=== Batch in Transaction ===");

  grovedb::test::TempDir dir("ex_batch_in_txn");
  auto result = grovedb::Db::Open(dir.PathToString());
  if (!result.has_value()) {
    Log(ERROR, "failed to open database: {}", result.error().message());
    return EXIT_FAILURE;
  }
  auto db = std::move(result).value();
  Log(INFO, "database opened at {}", dir.PathToString());

  grovedb::Path root{};

  // -------------------------------------------------------------------------
  // 1. Pre-insert a key so we can verify isolation.
  // -------------------------------------------------------------------------

  auto pre_elem = grovedb::Element::Item(grovedb::Bytes::FromString("pre_existing"));
  if (!pre_elem.has_value()) {
    Log(ERROR, "failed to create element: {}", pre_elem.error().message());
    return EXIT_FAILURE;
  }
  auto pre = db.Put(root, grovedb::Bytes::FromString("pre"), *pre_elem);
  if (!pre.has_value()) {
    Log(ERROR, "pre-insert failed: {}", pre.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "pre-inserted 'pre' = 'pre_existing'");

  // -------------------------------------------------------------------------
  // 2. Commit path: begin txn → batch 3 ops → verify isolation → commit.
  // -------------------------------------------------------------------------

  Log(INFO, "--- commit path ---");

  auto txn = db.BeginTransaction();
  if (!txn.has_value()) {
    Log(ERROR, "BeginTransaction failed: {}", txn.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "transaction started");

  auto e1 = grovedb::Element::Item(grovedb::Bytes::FromString("val_a"));
  auto e2 = grovedb::Element::Item(grovedb::Bytes::FromString("val_b"));
  auto e3 = grovedb::Element::Item(grovedb::Bytes::FromString("val_c"));
  if (!e1.has_value() || !e2.has_value() || !e3.has_value()) {
    Log(ERROR, "failed to create elements");
    return EXIT_FAILURE;
  }

  std::vector<grovedb::BatchOperation> ops{
      grovedb::BatchOperation::InsertOnly(root, {'a'}, *e1),
      grovedb::BatchOperation::InsertOnly(root, {'b'}, *e2),
      grovedb::BatchOperation::InsertOnly(root, {'c'}, *e3),
  };

  grovedb::BatchApplyOptions options;
  auto batch = db.ApplyBatch(ops, options, *txn);
  if (!batch.has_value()) {
    Log(ERROR, "ApplyBatch in txn failed: {}", batch.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "applied 3-op batch in transaction: {} seeks", batch->m_seek_count);

  // Verify isolation: keys not visible outside transaction.
  auto k_out = db.KeyExists(root, {'a'});
  Log(INFO, "KeyExists('a') outside txn: {}", k_out.has_value() ? k_out->value() : false);

  // Commit.
  auto commit = db.Commit(*txn);
  if (!commit.has_value()) {
    Log(ERROR, "commit failed: {}", commit.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "transaction committed: {} seeks", commit->m_seek_count);

  // Now visible.
  for (const char* k : {"a", "b", "c"}) {
    auto ke = db.KeyExists(root, grovedb::Bytes::FromString(k));
    Log(INFO, "KeyExists('{}') after commit: {}", k, ke.has_value() ? ke->value() : false);
  }

  // -------------------------------------------------------------------------
  // 3. Rollback path: begin txn → batch → rollback → verify absent.
  // -------------------------------------------------------------------------

  Log(INFO, "--- rollback path ---");

  auto txn2 = db.BeginTransaction();
  if (!txn2.has_value()) {
    Log(ERROR, "BeginTransaction failed: {}", txn2.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "transaction started");

  auto e_rb = grovedb::Element::Item(grovedb::Bytes::FromString("rollback_val"));
  if (!e_rb.has_value()) {
    Log(ERROR, "failed to create element: {}", e_rb.error().message());
    return EXIT_FAILURE;
  }

  std::vector<grovedb::BatchOperation> rb_ops{
      grovedb::BatchOperation::InsertOnly(root, grovedb::Bytes::FromString("rb_key"), *e_rb),
  };

  auto rb_batch = db.ApplyBatch(rb_ops, options, *txn2);
  if (!rb_batch.has_value()) {
    Log(ERROR, "ApplyBatch in txn failed: {}", rb_batch.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "applied batch in transaction");

  auto rb = db.Rollback(*txn2);
  if (!rb.has_value()) {
    Log(ERROR, "rollback failed: {}", rb.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "transaction rolled back");

  auto k_rb = db.KeyExists(root, grovedb::Bytes::FromString("rb_key"));
  Log(INFO, "KeyExists('rb_key') after rollback: {}", k_rb.has_value() ? k_rb->value() : false);

  // -------------------------------------------------------------------------
  // 4. RAII path: scope block → batch in txn → txn destroyed → verify absent.
  // -------------------------------------------------------------------------

  Log(INFO, "--- RAII auto-rollback path ---");
  {
    auto txn3 = db.BeginTransaction();
    if (!txn3.has_value()) {
      Log(ERROR, "BeginTransaction failed: {}", txn3.error().message());
      return EXIT_FAILURE;
    }
    Log(INFO, "transaction started (will go out of scope)");

    auto e_raii = grovedb::Element::Item(grovedb::Bytes::FromString("raii_val"));
    if (!e_raii.has_value()) {
      Log(ERROR, "failed to create element: {}", e_raii.error().message());
      return EXIT_FAILURE;
    }

    std::vector<grovedb::BatchOperation> raii_ops{
        grovedb::BatchOperation::InsertOnly(root, grovedb::Bytes::FromString("raii_key"), *e_raii),
    };

    auto raii_batch = db.ApplyBatch(raii_ops, options, *txn3);
    if (!raii_batch.has_value()) {
      Log(ERROR, "ApplyBatch in txn failed: {}", raii_batch.error().message());
      return EXIT_FAILURE;
    }
    Log(INFO, "applied batch in transaction");
    // txn3 goes out of scope here — auto-rollback.
  }

  auto k_raii = db.KeyExists(root, grovedb::Bytes::FromString("raii_key"));
  Log(INFO,
      "KeyExists('raii_key') after RAII destruction: {} (auto-rolled back)",
      k_raii.has_value() ? k_raii->value() : false);

  Log(INFO, "done");
  return EXIT_SUCCESS;
}

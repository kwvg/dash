// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <test/util/tempdir.h>

#include <grovedb/batch.h>
#include <grovedb/db.h>
#include <grovedb/element.h>

#include <bench/bench.h>

namespace {
/** Build a vector of InsertOnly BatchOperations. */
std::vector<grovedb::BatchOperation> MakeInsertOps(uint64_t start_idx, size_t count)
{
  std::vector<grovedb::BatchOperation> ops;
  ops.reserve(count);
  grovedb::Path root{};
  for (size_t i = 0; i < count; ++i) {
    auto key = grovedb::bench_util::MakeKey(start_idx + i);
    auto val = grovedb::bench_util::MakeValue(start_idx + i);
    auto elem = grovedb::Element::Item(val).value();
    ops.push_back(grovedb::BatchOperation::InsertOnly(root, key, elem));
  }
  return ops;
}

// ---------------------------------------------------------------------------
// Batch insert at various sizes
// ---------------------------------------------------------------------------

void BatchInsert10(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_batch_i10");
  auto db = grovedb::Db::Open(dir.PathToString()).value();
  grovedb::BatchApplyOptions opts;
  uint64_t idx = 0;

  auto ops0 = MakeInsertOps(idx, 10);
  auto pre = db.ApplyBatch(ops0, opts);
  grovedb::SetCostContext(bench, pre.value());
  idx += 10;

  bench.run("BatchInsert10", [&] {
    auto ops = MakeInsertOps(idx, 10);
    auto r = db.ApplyBatch(ops, opts);
    ankerl::nanobench::doNotOptimizeAway(r);
    idx += 10;
  });
}

void BatchInsert100(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_batch_i100");
  auto db = grovedb::Db::Open(dir.PathToString()).value();
  grovedb::BatchApplyOptions opts;
  uint64_t idx = 0;

  auto ops0 = MakeInsertOps(idx, 100);
  auto pre = db.ApplyBatch(ops0, opts);
  grovedb::SetCostContext(bench, pre.value());
  idx += 100;

  bench.run("BatchInsert100", [&] {
    auto ops = MakeInsertOps(idx, 100);
    auto r = db.ApplyBatch(ops, opts);
    ankerl::nanobench::doNotOptimizeAway(r);
    idx += 100;
  });
}

void BatchInsert1000(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_batch_i1k");
  auto db = grovedb::Db::Open(dir.PathToString()).value();
  grovedb::BatchApplyOptions opts;
  uint64_t idx = 0;

  auto ops0 = MakeInsertOps(idx, 1000);
  auto pre = db.ApplyBatch(ops0, opts);
  grovedb::SetCostContext(bench, pre.value());
  idx += 1000;

  bench.minEpochIterations(5);
  bench.run("BatchInsert1000", [&] {
    auto ops = MakeInsertOps(idx, 1000);
    auto r = db.ApplyBatch(ops, opts);
    ankerl::nanobench::doNotOptimizeAway(r);
    idx += 1000;
  });
}

// ---------------------------------------------------------------------------
// Mixed batch
// ---------------------------------------------------------------------------

void BatchMixed100(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_batch_mix");
  auto db = grovedb::Db::Open(dir.PathToString()).value();
  grovedb::bench_util::PopulateDb(db, /*n=*/10000);
  grovedb::BatchApplyOptions opts;

  uint64_t insert_idx = 100000;
  uint64_t delete_idx = 0;

  grovedb::ClearCostContext(bench);

  bench.run("BatchMixed100", [&] {
    std::vector<grovedb::BatchOperation> ops;
    ops.reserve(100);
    grovedb::Path root{};

    // 50 inserts.
    for (int i = 0; i < 50; ++i) {
      auto key = grovedb::bench_util::MakeKey(insert_idx);
      auto val = grovedb::bench_util::MakeValue(insert_idx);
      auto elem = grovedb::Element::Item(val).value();
      ops.push_back(grovedb::BatchOperation::InsertOnly(root, key, elem));
      ++insert_idx;
    }
    // 50 deletes.
    for (int i = 0; i < 50; ++i) {
      auto key = grovedb::bench_util::MakeKey(delete_idx);
      ops.push_back(grovedb::BatchOperation::Delete(root, key));
      ++delete_idx;
    }

    auto r = db.ApplyBatch(ops, opts);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

// ---------------------------------------------------------------------------
// Replace batch
// ---------------------------------------------------------------------------

void BatchReplace100(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_batch_repl");
  auto db = grovedb::Db::Open(dir.PathToString()).value();
  grovedb::bench_util::PopulateDb(db, /*n=*/10000);
  grovedb::BatchApplyOptions opts;

  uint64_t round = 0;

  grovedb::ClearCostContext(bench);

  bench.run("BatchReplace100", [&] {
    std::vector<grovedb::BatchOperation> ops;
    ops.reserve(100);
    grovedb::Path root{};
    for (int i = 0; i < 100; ++i) {
      auto key = grovedb::bench_util::MakeKey(static_cast<uint64_t>(i));
      auto val = grovedb::bench_util::MakeValue(round * 100 + static_cast<uint64_t>(i));
      auto elem = grovedb::Element::Item(val).value();
      ops.push_back(grovedb::BatchOperation::InsertOrReplace(root, key, elem));
    }
    auto r = db.ApplyBatch(ops, opts);
    ankerl::nanobench::doNotOptimizeAway(r);
    ++round;
  });
}

// ---------------------------------------------------------------------------
// Delete tree batch
// ---------------------------------------------------------------------------

void BatchDeleteTree(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_batch_deltree");
  auto db = grovedb::Db::Open(dir.PathToString()).value();
  grovedb::BatchApplyOptions opts;
  opts.m_allow_deleting_non_empty_trees = true;
  opts.m_deleting_non_empty_trees_returns_error = false;

  grovedb::Path root{};
  auto tree = grovedb::Element::EmptyTree().value();
  uint64_t round = 0;

  grovedb::ClearCostContext(bench);

  bench.run("BatchDeleteTree", [&] {
    // Create a subtree with some items.
    grovedb::Bytes sub_key = grovedb::bench_util::MakeKey(round, /*len=*/4);
    (void)db.Put(root, sub_key, tree);
    grovedb::Path sub{sub_key};
    for (int i = 0; i < 10; ++i) {
      auto key = grovedb::bench_util::MakeKey(static_cast<uint64_t>(i), /*len=*/8);
      auto elem = grovedb::Element::Item(grovedb::Bytes{'v'}).value();
      (void)db.Put(sub, key, elem);
    }

    // Delete it via batch.
    std::vector<grovedb::BatchOperation> ops{
        grovedb::BatchOperation::DeleteTree(root, sub_key),
    };
    auto r = db.ApplyBatch(ops, opts);
    ankerl::nanobench::doNotOptimizeAway(r);
    ++round;
  });
}

// ---------------------------------------------------------------------------
// Batch in transaction
// ---------------------------------------------------------------------------

void BatchInTxn100(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_batch_txn");
  auto db = grovedb::Db::Open(dir.PathToString()).value();
  grovedb::BatchApplyOptions opts;
  uint64_t idx = 0;

  grovedb::ClearCostContext(bench);

  bench.run("BatchInTxn100", [&] {
    auto txn = db.BeginTransaction().value();
    auto ops = MakeInsertOps(idx, 100);
    auto r = db.ApplyBatch(ops, opts, txn);
    ankerl::nanobench::doNotOptimizeAway(r);
    (void)db.Commit(txn);
    idx += 100;
  });
}

} // anonymous namespace

BENCHMARK(BatchInsert10);
BENCHMARK(BatchInsert100);
BENCHMARK(BatchInsert1000);
BENCHMARK(BatchMixed100);
BENCHMARK(BatchReplace100);
BENCHMARK(BatchDeleteTree);
BENCHMARK(BatchInTxn100);

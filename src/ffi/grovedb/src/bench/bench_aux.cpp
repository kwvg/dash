// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <test/util/tempdir.h>

#include <grovedb/db.h>
#include <grovedb/element.h>

#include <bench/bench.h>

namespace {
// ---------------------------------------------------------------------------
// Auxiliary storage
// ---------------------------------------------------------------------------

void PutAux(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_put_aux");
  auto db = grovedb::Db::Open(dir.PathToString()).value();

  uint64_t idx = 0;
  auto key0 = grovedb::bench_util::MakeKey(idx, /*len=*/8);
  auto val0 = grovedb::bench_util::MakeValue(idx);
  auto pre = db.PutAux(key0, val0);
  grovedb::SetCostContext(bench, pre.value());
  ++idx;

  bench.run("PutAux", [&] {
    auto key = grovedb::bench_util::MakeKey(idx, /*len=*/8);
    auto val = grovedb::bench_util::MakeValue(idx);
    auto r = db.PutAux(key, val);
    ankerl::nanobench::doNotOptimizeAway(r);
    ++idx;
  });
}

void GetAuxHit(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_getaux_hit");
  auto db = grovedb::Db::Open(dir.PathToString()).value();

  // Pre-populate aux storage.
  for (size_t i = 0; i < 1000; ++i) {
    auto key = grovedb::bench_util::MakeKey(i, /*len=*/8);
    auto val = grovedb::bench_util::MakeValue(i);
    (void)db.PutAux(key, val);
  }

  auto key = grovedb::bench_util::MakeKey(/*i=*/500, /*len=*/8);
  auto pre = db.GetAux(key);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("GetAuxHit", [&] {
    auto r = db.GetAux(key);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

void GetAuxMiss(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_getaux_miss");
  auto db = grovedb::Db::Open(dir.PathToString()).value();

  auto key = grovedb::bench_util::MakeKey(/*i=*/999999, /*len=*/8);
  auto pre = db.GetAux(key);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("GetAuxMiss", [&] {
    auto r = db.GetAux(key);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

void DeleteAux(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_del_aux");
  auto db = grovedb::Db::Open(dir.PathToString()).value();

  uint64_t idx = 0;

  // Pre-run for cost.
  auto key0 = grovedb::bench_util::MakeKey(idx, /*len=*/8);
  auto val0 = grovedb::bench_util::MakeValue(idx);
  (void)db.PutAux(key0, val0);
  auto pre = db.DeleteAux(key0);
  grovedb::SetCostContext(bench, pre.value());
  ++idx;

  bench.run("DeleteAux", [&] {
    auto key = grovedb::bench_util::MakeKey(idx, /*len=*/8);
    auto val = grovedb::bench_util::MakeValue(idx);
    (void)db.PutAux(key, val);
    auto r = db.DeleteAux(key);
    ankerl::nanobench::doNotOptimizeAway(r);
    ++idx;
  });
}

// ---------------------------------------------------------------------------
// Find subtrees
// ---------------------------------------------------------------------------

void FindSubtrees(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_find_sub");
  auto db = grovedb::Db::Open(dir.PathToString()).value();

  // Create 100 subtrees under root.
  grovedb::Path root{};
  auto tree = grovedb::Element::EmptyTree().value();
  for (size_t i = 0; i < 100; ++i) {
    auto key = grovedb::bench_util::MakeKey(i, /*len=*/8);
    (void)db.Put(root, key, tree);
  }

  auto pre = db.FindSubtrees(root);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("FindSubtrees", [&] {
    auto r = db.FindSubtrees(root);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

// ---------------------------------------------------------------------------
// Checkpoint
// ---------------------------------------------------------------------------

void CreateCheckpoint(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_ckpt");
  auto db = grovedb::Db::Open(dir.PathToString()).value();
  grovedb::bench_util::PopulateDb(db, /*n=*/1000);

  // TempDir for checkpoint targets.
  grovedb::test::TempDir ckpt_parent("bench_ckpt_targets");
  uint64_t idx = 0;

  grovedb::ClearCostContext(bench);

  bench.run("CreateCheckpoint", [&] {
    // CreateCheckpoint expects target dir to NOT exist.
    std::string target = ckpt_parent.PathToString() + "/ckpt_" + std::to_string(idx);
    auto r = db.CreateCheckpoint(target);
    ankerl::nanobench::doNotOptimizeAway(r);
    ++idx;
  });
}
} // anonymous namespace

BENCHMARK(PutAux);
BENCHMARK(GetAuxHit);
BENCHMARK(GetAuxMiss);
BENCHMARK(DeleteAux);
BENCHMARK(FindSubtrees);
BENCHMARK(CreateCheckpoint);

// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <test/util/tempdir.h>

#include <grovedb/db.h>
#include <grovedb/element.h>

#include <bench/bench.h>

namespace {
void Delete(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_delete");
  auto db = grovedb::Db::Open(dir.PathToString()).value();
  grovedb::bench_util::PopulateDb(db, /*n=*/10000);

  grovedb::Path path{};
  uint64_t idx = 0;

  // Pre-run for cost.
  auto key0 = grovedb::bench_util::MakeKey(idx);
  auto pre = db.Delete(path, key0);
  grovedb::SetCostContext(bench, pre.value());
  ++idx;

  bench.run("Delete", [&] {
    auto key = grovedb::bench_util::MakeKey(idx);
    auto r = db.Delete(path, key);
    ankerl::nanobench::doNotOptimizeAway(r);
    ++idx;
  });
}

void DeleteIfEmptyTrue(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_del_empty_t");
  auto db = grovedb::Db::Open(dir.PathToString()).value();

  grovedb::Path root{};
  auto tree = grovedb::Element::EmptyTree().value();
  uint64_t idx = 0;

  // Pre-run: create and delete to get cost.
  auto key0 = grovedb::bench_util::MakeKey(idx);
  (void)db.Put(root, key0, tree);
  auto pre = db.DeleteIfEmpty(root, key0);
  grovedb::SetCostContext(bench, pre.value().m_cost);
  ++idx;

  bench.run("DeleteIfEmptyTrue", [&] {
    auto key = grovedb::bench_util::MakeKey(idx);
    (void)db.Put(root, key, tree);
    auto r = db.DeleteIfEmpty(root, key);
    ankerl::nanobench::doNotOptimizeAway(r);
    ++idx;
  });
}

void DeleteIfEmptyFalse(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_del_empty_f");
  auto db = grovedb::Db::Open(dir.PathToString()).value();

  grovedb::Path root{};
  grovedb::Bytes subtree_key{'s'};
  auto tree = grovedb::Element::EmptyTree().value();
  (void)db.Put(root, subtree_key, tree);

  // Put an item in the subtree so it's non-empty.
  grovedb::Path subtree{subtree_key};
  auto item = grovedb::Element::Item(grovedb::Bytes{'v'}).value();
  (void)db.Put(subtree, {'i'}, item);

  auto pre = db.DeleteIfEmpty(root, subtree_key);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("DeleteIfEmptyFalse", [&] {
    auto r = db.DeleteIfEmpty(root, subtree_key);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

void PruneEmptyAncestors(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_prune");
  auto db = grovedb::Db::Open(dir.PathToString()).value();

  grovedb::Path root{};
  auto tree = grovedb::Element::EmptyTree().value();
  uint64_t idx = 0;

  // Pre-run: create 3-level chain and prune.
  grovedb::Bytes k_a = grovedb::bench_util::MakeKey(idx, /*len=*/4);
  (void)db.Put(root, k_a, tree);
  grovedb::Path p1{k_a};
  grovedb::Bytes k_b{'b'};
  (void)db.Put(p1, k_b, tree);
  grovedb::Path p2{k_a, k_b};
  grovedb::Bytes k_c{'c'};
  auto leaf = grovedb::Element::Item(grovedb::Bytes{'v'}).value();
  (void)db.Put(p2, k_c, leaf);
  auto pre = db.PruneEmptyAncestors(p2, k_c);
  grovedb::SetCostContext(bench, pre.value().m_cost);
  ++idx;

  bench.run("PruneEmptyAncestors", [&] {
    // Rebuild chain each iteration.
    grovedb::Bytes ka = grovedb::bench_util::MakeKey(idx, /*len=*/4);
    (void)db.Put(root, ka, tree);
    grovedb::Path pa{ka};
    (void)db.Put(pa, k_b, tree);
    grovedb::Path pb{ka, k_b};
    (void)db.Put(pb, k_c, leaf);
    auto r = db.PruneEmptyAncestors(pb, k_c);
    ankerl::nanobench::doNotOptimizeAway(r);
    ++idx;
  });
}

void Clear(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_clear");
  auto db = grovedb::Db::Open(dir.PathToString()).value();

  grovedb::Path root{};
  grovedb::Bytes subtree_key{'s'};
  auto tree = grovedb::Element::EmptyTree().value();
  uint64_t round = 0;

  grovedb::ClearCostContext(bench);

  bench.run("Clear", [&] {
    // Create subtree with items, then clear it.
    (void)db.Put(root, subtree_key, tree);
    grovedb::Path sub{subtree_key};
    for (int i = 0; i < 100; ++i) {
      auto key = grovedb::bench_util::MakeKey(round * 100 + static_cast<uint64_t>(i), /*len=*/8);
      auto elem = grovedb::Element::Item(grovedb::Bytes{'v'}).value();
      (void)db.Put(sub, key, elem);
    }
    auto r = db.Clear(sub);
    ankerl::nanobench::doNotOptimizeAway(r);
    // Clean up subtree for next iteration.
    (void)db.Delete(root, subtree_key);
    ++round;
  });
}

} // anonymous namespace

BENCHMARK(Delete);
BENCHMARK(DeleteIfEmptyTrue);
BENCHMARK(DeleteIfEmptyFalse);
BENCHMARK(PruneEmptyAncestors);
BENCHMARK(Clear);

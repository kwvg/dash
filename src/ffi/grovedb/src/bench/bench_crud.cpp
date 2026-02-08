// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <test/util/tempdir.h>

#include <grovedb/db.h>
#include <grovedb/element.h>

#include <bench/bench.h>

namespace {
// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void Open(ankerl::nanobench::Bench& bench)
{
  grovedb::ClearCostContext(bench);

  bench.run("Open", [&] {
    grovedb::test::TempDir dir("bench_open");
    auto r = grovedb::Db::Open(dir.PathToString());
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

void Flush(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_flush");
  auto db = grovedb::Db::Open(dir.PathToString()).value();
  grovedb::bench_util::PopulateDb(db, /*n=*/1000);

  grovedb::ClearCostContext(bench);

  bench.run("Flush", [&] {
    auto r = db.Flush();
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

void GetRootHash(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_root_hash");
  auto db = grovedb::Db::Open(dir.PathToString()).value();
  grovedb::bench_util::PopulateDb(db, /*n=*/1000);

  auto pre = db.GetRootHash();
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("GetRootHash", [&] {
    auto r = db.GetRootHash();
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

// ---------------------------------------------------------------------------
// Put
// ---------------------------------------------------------------------------

void PutSequential(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_put_seq");
  auto db = grovedb::Db::Open(dir.PathToString()).value();

  grovedb::Path path{};
  uint64_t idx = 0;

  // Pre-run for cost.
  auto key0 = grovedb::bench_util::MakeKey(idx);
  auto val0 = grovedb::bench_util::MakeValue(idx);
  auto elem0 = grovedb::Element::Item(val0).value();
  auto pre = db.Put(path, key0, elem0);
  grovedb::SetCostContext(bench, pre.value());
  ++idx;

  bench.run("PutSequential", [&] {
    auto key = grovedb::bench_util::MakeKey(idx);
    auto val = grovedb::bench_util::MakeValue(idx);
    auto elem = grovedb::Element::Item(val).value();
    auto r = db.Put(path, key, elem);
    ankerl::nanobench::doNotOptimizeAway(r);
    ++idx;
  });
}

void PutRandom(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_put_rnd");
  auto db = grovedb::Db::Open(dir.PathToString()).value();
  grovedb::bench_util::PopulateDb(db, /*n=*/10000);

  grovedb::Path path{};
  uint64_t idx = 100000;

  auto key0 = grovedb::bench_util::MakeKey(idx);
  auto val0 = grovedb::bench_util::MakeValue(idx);
  auto elem0 = grovedb::Element::Item(val0).value();
  auto pre = db.Put(path, key0, elem0);
  grovedb::SetCostContext(bench, pre.value());
  ++idx;

  bench.run("PutRandom", [&] {
    auto key = grovedb::bench_util::MakeKey(idx);
    auto val = grovedb::bench_util::MakeValue(idx);
    auto elem = grovedb::Element::Item(val).value();
    auto r = db.Put(path, key, elem);
    ankerl::nanobench::doNotOptimizeAway(r);
    ++idx;
  });
}

// ---------------------------------------------------------------------------
// Get
// ---------------------------------------------------------------------------

void GetHit(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_get_hit");
  auto db = grovedb::Db::Open(dir.PathToString()).value();
  grovedb::bench_util::PopulateDb(db, /*n=*/10000);

  grovedb::Path path{};
  auto key = grovedb::bench_util::MakeKey(/*i=*/500);

  auto pre = db.Get(path, key);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("GetHit", [&] {
    auto r = db.Get(path, key);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

void GetMiss(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_get_miss");
  auto db = grovedb::Db::Open(dir.PathToString()).value();
  grovedb::bench_util::PopulateDb(db, /*n=*/10000);

  grovedb::Path path{};
  auto key = grovedb::bench_util::MakeKey(/*i=*/999999);

  grovedb::ClearCostContext(bench);

  bench.run("GetMiss", [&] {
    auto r = db.Get(path, key);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

void GetDirect(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_get_direct");
  auto db = grovedb::Db::Open(dir.PathToString()).value();
  grovedb::bench_util::PopulateDb(db, /*n=*/10000);

  grovedb::Path path{};
  auto key = grovedb::bench_util::MakeKey(/*i=*/500);

  auto pre = db.GetDirect(path, key);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("GetDirect", [&] {
    auto r = db.GetDirect(path, key);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

void GetOptionalHit(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_getopt_hit");
  auto db = grovedb::Db::Open(dir.PathToString()).value();
  grovedb::bench_util::PopulateDb(db, /*n=*/10000);

  grovedb::Path path{};
  auto key = grovedb::bench_util::MakeKey(/*i=*/500);

  auto pre = db.GetOptional(path, key);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("GetOptionalHit", [&] {
    auto r = db.GetOptional(path, key);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

void GetOptionalMiss(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_getopt_miss");
  auto db = grovedb::Db::Open(dir.PathToString()).value();
  grovedb::bench_util::PopulateDb(db, /*n=*/10000);

  grovedb::Path path{};
  auto key = grovedb::bench_util::MakeKey(/*i=*/999999);

  auto pre = db.GetOptional(path, key);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("GetOptionalMiss", [&] {
    auto r = db.GetOptional(path, key);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

// ---------------------------------------------------------------------------
// Key existence
// ---------------------------------------------------------------------------

void KeyExistsHit(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_kexist_hit");
  auto db = grovedb::Db::Open(dir.PathToString()).value();
  grovedb::bench_util::PopulateDb(db, /*n=*/10000);

  grovedb::Path path{};
  auto key = grovedb::bench_util::MakeKey(/*i=*/500);

  auto pre = db.KeyExists(path, key);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("KeyExistsHit", [&] {
    auto r = db.KeyExists(path, key);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

void KeyExistsMiss(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_kexist_miss");
  auto db = grovedb::Db::Open(dir.PathToString()).value();
  grovedb::bench_util::PopulateDb(db, /*n=*/10000);

  grovedb::Path path{};
  auto key = grovedb::bench_util::MakeKey(/*i=*/999999);

  auto pre = db.KeyExists(path, key);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("KeyExistsMiss", [&] {
    auto r = db.KeyExists(path, key);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

// ---------------------------------------------------------------------------
// Subtree / tree state
// ---------------------------------------------------------------------------

void SubtreeExists(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_subtree_exists");
  auto db = grovedb::Db::Open(dir.PathToString()).value();

  // Create a 3-level subtree: root -> "a" -> "b" -> "c"
  grovedb::Path root{};
  auto tree = grovedb::Element::EmptyTree().value();
  (void)db.Put(root, {'a'}, tree);
  (void)db.Put(grovedb::Path{{'a'}}, {'b'}, tree);
  (void)db.Put(grovedb::Path{{'a'}, {'b'}}, {'c'}, tree);

  grovedb::Path check{{'a'}, {'b'}, {'c'}};
  auto pre = db.SubtreeExists(check);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("SubtreeExists", [&] {
    auto r = db.SubtreeExists(check);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

void IsEmptyTree(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_is_empty_tree");
  auto db = grovedb::Db::Open(dir.PathToString()).value();

  // Root path, populated with items.
  grovedb::bench_util::PopulateDb(db, /*n=*/100);
  grovedb::Path root{};

  auto pre = db.IsEmptyTree(root);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("IsEmptyTree", [&] {
    auto r = db.IsEmptyTree(root);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

// ---------------------------------------------------------------------------
// Conditional inserts
// ---------------------------------------------------------------------------

void PutIfAbsentNew(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_pifabsent_new");
  auto db = grovedb::Db::Open(dir.PathToString()).value();

  grovedb::Path path{};
  uint64_t idx = 0;

  auto key0 = grovedb::bench_util::MakeKey(idx);
  auto val0 = grovedb::bench_util::MakeValue(idx);
  auto elem0 = grovedb::Element::Item(val0).value();
  auto pre = db.PutIfAbsent(path, key0, elem0);
  grovedb::SetCostContext(bench, pre.value().m_cost);
  ++idx;

  bench.run("PutIfAbsentNew", [&] {
    auto key = grovedb::bench_util::MakeKey(idx);
    auto val = grovedb::bench_util::MakeValue(idx);
    auto elem = grovedb::Element::Item(val).value();
    auto r = db.PutIfAbsent(path, key, elem);
    ankerl::nanobench::doNotOptimizeAway(r);
    ++idx;
  });
}

void PutIfAbsentExists(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_pifabsent_dup");
  auto db = grovedb::Db::Open(dir.PathToString()).value();

  grovedb::Path path{};
  auto key = grovedb::bench_util::MakeKey(/*i=*/0);
  auto val = grovedb::bench_util::MakeValue(/*i=*/0);
  auto elem = grovedb::Element::Item(val).value();
  (void)db.Put(path, key, elem);

  auto pre = db.PutIfAbsent(path, key, elem);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("PutIfAbsentExists", [&] {
    auto r = db.PutIfAbsent(path, key, elem);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

void PutIfChangedSame(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_pifchanged_same");
  auto db = grovedb::Db::Open(dir.PathToString()).value();

  grovedb::Path path{};
  auto key = grovedb::bench_util::MakeKey(/*i=*/0);
  auto val = grovedb::bench_util::MakeValue(/*i=*/0);
  auto elem = grovedb::Element::Item(val).value();
  (void)db.Put(path, key, elem);

  auto pre = db.PutIfChanged(path, key, elem);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("PutIfChangedSame", [&] {
    auto r = db.PutIfChanged(path, key, elem);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

void PutIfChangedDiff(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_pifchanged_diff");
  auto db = grovedb::Db::Open(dir.PathToString()).value();

  grovedb::Path path{};
  auto key = grovedb::bench_util::MakeKey(/*i=*/0);
  auto val1 = grovedb::bench_util::MakeValue(/*i=*/0);
  auto val2 = grovedb::bench_util::MakeValue(/*i=*/1);
  auto elem1 = grovedb::Element::Item(val1).value();
  auto elem2 = grovedb::Element::Item(val2).value();
  (void)db.Put(path, key, elem1);

  auto pre = db.PutIfChanged(path, key, elem2);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("PutIfChangedDiff", [&] {
    // Alternate: write elem2, then elem1 to always trigger a change.
    auto r = db.PutIfChanged(path, key, elem2);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

// ---------------------------------------------------------------------------
// Transactions
// ---------------------------------------------------------------------------

void TxnBeginCommitEmpty(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_txn_empty");
  auto db = grovedb::Db::Open(dir.PathToString()).value();

  grovedb::ClearCostContext(bench);

  bench.run("TxnBeginCommitEmpty", [&] {
    auto txn = db.BeginTransaction().value();
    auto r = db.Commit(txn);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

void TxnPutCommit(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_txn_put");
  auto db = grovedb::Db::Open(dir.PathToString()).value();

  grovedb::Path path{};
  uint64_t idx = 0;

  grovedb::ClearCostContext(bench);

  bench.run("TxnPutCommit", [&] {
    auto txn = db.BeginTransaction().value();
    for (int i = 0; i < 10; ++i) {
      auto key = grovedb::bench_util::MakeKey(idx);
      auto val = grovedb::bench_util::MakeValue(idx);
      auto elem = grovedb::Element::Item(val).value();
      (void)db.Put(path, key, elem, txn);
      ++idx;
    }
    auto r = db.Commit(txn);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

void TxnGetOverhead(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_txn_get");
  auto db = grovedb::Db::Open(dir.PathToString()).value();
  grovedb::bench_util::PopulateDb(db, /*n=*/10000);

  grovedb::Path path{};
  auto key = grovedb::bench_util::MakeKey(/*i=*/500);

  auto txn = db.BeginTransaction().value();
  auto pre = db.Get(path, key, txn);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("TxnGetOverhead", [&] {
    auto r = db.Get(path, key, txn);
    ankerl::nanobench::doNotOptimizeAway(r);
  });

  (void)db.Rollback(txn);
}
} // anonymous namespace

BENCHMARK(Open);
BENCHMARK(Flush);
BENCHMARK(GetRootHash);
BENCHMARK(PutSequential);
BENCHMARK(PutRandom);
BENCHMARK(GetHit);
BENCHMARK(GetMiss);
BENCHMARK(GetDirect);
BENCHMARK(GetOptionalHit);
BENCHMARK(GetOptionalMiss);
BENCHMARK(KeyExistsHit);
BENCHMARK(KeyExistsMiss);
BENCHMARK(SubtreeExists);
BENCHMARK(IsEmptyTree);
BENCHMARK(PutIfAbsentNew);
BENCHMARK(PutIfAbsentExists);
BENCHMARK(PutIfChangedSame);
BENCHMARK(PutIfChangedDiff);
BENCHMARK(TxnBeginCommitEmpty);
BENCHMARK(TxnPutCommit);
BENCHMARK(TxnGetOverhead);

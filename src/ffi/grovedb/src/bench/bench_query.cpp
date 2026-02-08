// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <test/util/tempdir.h>

#include <grovedb/db.h>
#include <grovedb/element.h>
#include <grovedb/query.h>

#include <bench/bench.h>

namespace {
/** Open a DB and populate it with n Item entries at the root path. */
grovedb::Db MakePopulatedDb(grovedb::test::TempDir& dir, size_t n)
{
  auto db = grovedb::Db::Open(dir.PathToString()).value();
  grovedb::bench_util::PopulateDb(db, n);
  return db;
}

// ---------------------------------------------------------------------------
// QueryValues
// ---------------------------------------------------------------------------

void QueryValuesKey(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_qv_key");
  auto db = MakePopulatedDb(dir, /*n=*/10000);

  grovedb::Path root{};
  auto key = grovedb::bench_util::MakeKey(/*i=*/500);
  auto query = grovedb::PathQuery::New(root, {grovedb::QueryItem::Key(key)}).value();

  auto pre = db.QueryValues(query);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("QueryValuesKey", [&] {
    auto r = db.QueryValues(query);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

void QueryValuesRange10(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_qv_r10");
  auto db = MakePopulatedDb(dir, /*n=*/1000);

  grovedb::Path root{};
  auto query =
      grovedb::PathQuery::New(root, {grovedb::QueryItem::RangeFull()}, /*limit=*/100).value();

  auto pre = db.QueryValues(query);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("QueryValuesRange10", [&] {
    auto r = db.QueryValues(query);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

void QueryValuesRange100(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_qv_r100");
  auto db = MakePopulatedDb(dir, /*n=*/100);

  grovedb::Path root{};
  auto query = grovedb::PathQuery::New(root, {grovedb::QueryItem::RangeFull()}).value();

  auto pre = db.QueryValues(query);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("QueryValuesRange100", [&] {
    auto r = db.QueryValues(query);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

void QueryValuesLimit(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_qv_limit");
  auto db = MakePopulatedDb(dir, /*n=*/10000);

  grovedb::Path root{};
  auto query =
      grovedb::PathQuery::New(root, {grovedb::QueryItem::RangeFull()}, /*limit=*/10).value();

  auto pre = db.QueryValues(query);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("QueryValuesLimit", [&] {
    auto r = db.QueryValues(query);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

// ---------------------------------------------------------------------------
// QueryRaw
// ---------------------------------------------------------------------------

void QueryRawElement(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_qr_elem");
  auto db = MakePopulatedDb(dir, /*n=*/1000);

  grovedb::Path root{};
  auto query =
      grovedb::PathQuery::New(root, {grovedb::QueryItem::RangeFull()}, /*limit=*/10).value();

  auto pre = db.QueryRaw(query, /*result_type=*/0);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("QueryRawElement", [&] {
    auto r = db.QueryRaw(query, /*result_type=*/0);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

void QueryRawKeyElement(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_qr_ke");
  auto db = MakePopulatedDb(dir, /*n=*/1000);

  grovedb::Path root{};
  auto query =
      grovedb::PathQuery::New(root, {grovedb::QueryItem::RangeFull()}, /*limit=*/10).value();

  auto pre = db.QueryRaw(query, /*result_type=*/1);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("QueryRawKeyElement", [&] {
    auto r = db.QueryRaw(query, /*result_type=*/1);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

void QueryRawPathKeyElement(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_qr_pke");
  auto db = MakePopulatedDb(dir, /*n=*/1000);

  grovedb::Path root{};
  auto query =
      grovedb::PathQuery::New(root, {grovedb::QueryItem::RangeFull()}, /*limit=*/10).value();

  auto pre = db.QueryRaw(query, /*result_type=*/2);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("QueryRawPathKeyElement", [&] {
    auto r = db.QueryRaw(query, /*result_type=*/2);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

// ---------------------------------------------------------------------------
// QuerySums
// ---------------------------------------------------------------------------

void QuerySums(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_qsums");
  auto db = grovedb::Db::Open(dir.PathToString()).value();

  // Create a sum tree and populate with SumItems.
  grovedb::Path root{};
  grovedb::Bytes sum_key{'s'};
  auto sum_tree = grovedb::Element::EmptySumTree().value();
  (void)db.Put(root, sum_key, sum_tree);

  grovedb::Path sum_path{sum_key};
  for (size_t i = 0; i < 1000; ++i) {
    auto key = grovedb::bench_util::MakeKey(i, /*len=*/8);
    auto elem = grovedb::Element::SumItem(static_cast<int64_t>(i)).value();
    (void)db.Put(sum_path, key, elem);
  }

  auto query = grovedb::PathQuery::New(sum_path, {grovedb::QueryItem::RangeFull()}).value();
  auto pre = db.QuerySums(query);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("QuerySums", [&] {
    auto r = db.QuerySums(query);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

// ---------------------------------------------------------------------------
// QueryItemsOrSums
// ---------------------------------------------------------------------------

void QueryItemsOrSums(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_qios");
  auto db = grovedb::Db::Open(dir.PathToString()).value();

  // Create a sum tree with sum items.
  grovedb::Path root{};
  grovedb::Bytes sum_key{'m'};
  auto sum_tree = grovedb::Element::EmptySumTree().value();
  (void)db.Put(root, sum_key, sum_tree);

  grovedb::Path sum_path{sum_key};
  for (size_t i = 0; i < 100; ++i) {
    auto key = grovedb::bench_util::MakeKey(i, /*len=*/8);
    auto elem = grovedb::Element::SumItem(static_cast<int64_t>(i)).value();
    (void)db.Put(sum_path, key, elem);
  }

  auto query =
      grovedb::PathQuery::New(sum_path, {grovedb::QueryItem::RangeFull()}, /*limit=*/10).value();
  auto pre = db.QueryItemsOrSums(query);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("QueryItemsOrSums", [&] {
    auto r = db.QueryItemsOrSums(query);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

// ---------------------------------------------------------------------------
// QueryKeysOptional
// ---------------------------------------------------------------------------

void QueryKeysOptional(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_qko");
  auto db = MakePopulatedDb(dir, /*n=*/1000);

  grovedb::Path root{};
  // Mix of hit and miss keys using Key items.
  std::vector<grovedb::QueryItem> items;
  for (int i = 0; i < 10; ++i) {
    auto key = grovedb::bench_util::MakeKey(
        i % 2 == 0 ? static_cast<uint64_t>(i * 100) : 999999u + static_cast<uint64_t>(i)
    );
    items.push_back(grovedb::QueryItem::Key(key));
  }

  auto query = grovedb::PathQuery::New(root, items, /*limit=*/100).value();
  auto pre = db.QueryKeysOptional(query);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("QueryKeysOptional", [&] {
    auto r = db.QueryKeysOptional(query);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

} // anonymous namespace

BENCHMARK(QueryValuesKey);
BENCHMARK(QueryValuesRange10);
BENCHMARK(QueryValuesRange100);
BENCHMARK(QueryValuesLimit);
BENCHMARK(QueryRawElement);
BENCHMARK(QueryRawKeyElement);
BENCHMARK(QueryRawPathKeyElement);
BENCHMARK(QuerySums);
BENCHMARK(QueryItemsOrSums);
BENCHMARK(QueryKeysOptional);

// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <test/util/tempdir.h>

#include <grovedb/db.h>
#include <grovedb/element.h>
#include <grovedb/query.h>

#include <bench/bench.h>

namespace {
grovedb::Db MakeProofDb(grovedb::test::TempDir& dir)
{
  auto db = grovedb::Db::Open(dir.PathToString()).value();
  grovedb::bench_util::PopulateDb(db, /*n=*/1000);
  return db;
}

// ---------------------------------------------------------------------------
// Prove
// ---------------------------------------------------------------------------

void ProveSingleKey(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_prove_key");
  auto db = MakeProofDb(dir);

  grovedb::Path root{};
  auto key = grovedb::bench_util::MakeKey(/*i=*/500);
  auto query = grovedb::PathQuery::New(root, {grovedb::QueryItem::Key(key)}, /*limit=*/1).value();

  auto pre = db.Prove(query);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("ProveSingleKey", [&] {
    auto r = db.Prove(query);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

void ProveRange(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_prove_range");
  auto db = MakeProofDb(dir);

  grovedb::Path root{};
  auto start = grovedb::bench_util::MakeKey(/*i=*/0);
  auto end = grovedb::bench_util::MakeKey(/*i=*/100);
  auto query = grovedb::PathQuery::New(root, {grovedb::QueryItem::Range(start, end)}).value();

  auto pre = db.Prove(query);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("ProveRange", [&] {
    auto r = db.Prove(query);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

// ---------------------------------------------------------------------------
// Verify
// ---------------------------------------------------------------------------

void VerifyQuery(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_verify");
  auto db = MakeProofDb(dir);

  grovedb::Path root{};
  auto key = grovedb::bench_util::MakeKey(/*i=*/500);
  auto query = grovedb::PathQuery::New(root, {grovedb::QueryItem::Key(key)}, /*limit=*/1).value();

  auto proof = db.Prove(query).value().m_value;

  auto pre = db.VerifyQuery(proof, query);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("VerifyQuery", [&] {
    auto r = db.VerifyQuery(proof, query);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

void VerifySubsetQuery(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_verify_sub");
  auto db = MakeProofDb(dir);

  grovedb::Path root{};
  auto query =
      grovedb::PathQuery::New(root, {grovedb::QueryItem::RangeFull()}, /*limit=*/10).value();

  auto proof = db.Prove(query).value().m_value;

  auto pre = db.VerifySubsetQuery(proof, query);
  grovedb::SetCostContext(bench, pre.value().m_cost);

  bench.run("VerifySubsetQuery", [&] {
    auto r = db.VerifySubsetQuery(proof, query);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

// ---------------------------------------------------------------------------
// Round-trip
// ---------------------------------------------------------------------------

void ProveVerifyRoundTrip(ankerl::nanobench::Bench& bench)
{
  grovedb::test::TempDir dir("bench_pv_rt");
  auto db = MakeProofDb(dir);

  grovedb::Path root{};
  auto key = grovedb::bench_util::MakeKey(/*i=*/500);
  auto query = grovedb::PathQuery::New(root, {grovedb::QueryItem::Key(key)}, /*limit=*/1).value();

  grovedb::ClearCostContext(bench);

  bench.run("ProveVerifyRoundTrip", [&] {
    auto proof = db.Prove(query).value().m_value;
    auto r = db.VerifyQuery(proof, query);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

} // anonymous namespace

BENCHMARK(ProveSingleKey);
BENCHMARK(ProveRange);
BENCHMARK(VerifyQuery);
BENCHMARK(VerifySubsetQuery);
BENCHMARK(ProveVerifyRoundTrip);

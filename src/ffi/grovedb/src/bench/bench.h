// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#ifndef GROVEDB_BENCH_BENCH_H
#define GROVEDB_BENCH_BENCH_H

#include <grovedb/cost.h>
#include <grovedb/db.h>
#include <grovedb/element.h>
#include <grovedb/types.h>

#include <bench/vendor/nanobench.h>

#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <string>

namespace benchmark {
using BenchFunction = std::function<void(ankerl::nanobench::Bench&)>;

struct Args {
  bool is_list_only{false};
  std::string regex_filter;
};

class BenchRunner
{
public:
  using BenchmarkMap = std::map<std::string, BenchFunction>;
  BenchRunner(std::string name, BenchFunction func);
  static BenchmarkMap& benchmarks();
  static void RunAll(const Args& args);
};

} // namespace benchmark

#define BENCHMARK(n) static benchmark::BenchRunner bench_##n(#n, n);

namespace grovedb {
/** Set OperationCost fields as bench context for the custom render template. */
void SetCostContext(ankerl::nanobench::Bench& bench, const OperationCost& cost);

/** Clear cost context fields (used for benchmarks without cost data). */
void ClearCostContext(ankerl::nanobench::Bench& bench);

/** Render all results as a markdown table with cost columns. */
void RenderWithCosts(const std::vector<ankerl::nanobench::Result>& results, std::ostream& out);

namespace bench_util {
/** Generate a deterministic key of the given length for index i. */
Bytes MakeKey(uint64_t i, size_t len = 16);

/** Generate a deterministic value of the given length for index i. */
Bytes MakeValue(uint64_t i, size_t len = 100);

/** Pre-populate a DB with n sequential Item entries at the root path. */
void PopulateDb(Db& db, size_t n, size_t key_len = 16, size_t val_len = 100);
} // namespace bench_util
} // namespace grovedb

#endif // GROVEDB_BENCH_BENCH_H

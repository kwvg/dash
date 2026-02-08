// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#define ANKERL_NANOBENCH_IMPLEMENT
#include <bench/bench.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace benchmark {
BenchRunner::BenchmarkMap& BenchRunner::benchmarks()
{
  static BenchmarkMap map;
  return map;
}

BenchRunner::BenchRunner(std::string name, BenchFunction func)
{
  benchmarks().emplace(std::move(name), std::move(func));
}

void BenchRunner::RunAll(const Args& args)
{
  std::regex filter(args.regex_filter.empty() ? ".*" : args.regex_filter);
  std::smatch match;

  std::vector<ankerl::nanobench::Result> all_results;

  for (const auto& [name, func] : benchmarks()) {
    if (!std::regex_match(name, match, filter)) {
      continue;
    }
    if (args.is_list_only) {
      std::cout << name << std::endl;
      continue;
    }

    ankerl::nanobench::Bench bench;
    bench.output(nullptr);
    bench.name(name);
    func(bench);

    for (const auto& r : bench.results()) {
      all_results.push_back(r);
    }
  }

  if (!args.is_list_only && !all_results.empty()) {
    grovedb::RenderWithCosts(all_results, std::cout);
  }
}

} // namespace benchmark

// ---------------------------------------------------------------------------
// Cost context helpers
// ---------------------------------------------------------------------------

namespace grovedb {
void SetCostContext(ankerl::nanobench::Bench& bench, const OperationCost& cost)
{
  bench.context("seeks", std::to_string(cost.m_seek_count));
  bench.context("loaded", std::to_string(cost.m_storage_loaded_bytes));
  bench.context("hashes", std::to_string(cost.m_hash_node_calls));
}

void ClearCostContext(ankerl::nanobench::Bench& bench)
{
  bench.context("seeks", "-");
  bench.context("loaded", "-");
  bench.context("hashes", "-");
}

// ---------------------------------------------------------------------------
// Custom markdown table renderer with cost columns
// ---------------------------------------------------------------------------

namespace {
/** Format a double with comma-separated thousands. */
std::string FormatComma(double val, int precision)
{
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(precision) << val;
  std::string s = oss.str();

  // Find the decimal point or end.
  auto dot = s.find('.');
  std::string integer_part = s.substr(0, dot);
  std::string decimal_part = dot != std::string::npos ? s.substr(dot) : "";

  // Handle negative numbers.
  bool negative = !integer_part.empty() && integer_part[0] == '-';
  if (negative) {
    integer_part = integer_part.substr(1);
  }

  // Insert commas.
  std::string result;
  int count = 0;
  for (auto it = integer_part.rbegin(); it != integer_part.rend(); ++it) {
    if (count > 0 && count % 3 == 0) {
      result += ',';
    }
    result += *it;
    ++count;
  }
  std::reverse(result.begin(), result.end());
  if (negative) {
    result = "-" + result;
  }
  return result + decimal_part;
}

/** Format a duration in seconds to a human-readable string. */
std::string FormatDuration(double seconds)
{
  std::ostringstream oss;
  if (seconds < 1.0) {
    oss << std::fixed << std::setprecision(2) << seconds << " s";
  } else if (seconds < 60.0) {
    oss << std::fixed << std::setprecision(1) << seconds << " s";
  } else {
    oss << std::fixed << std::setprecision(0) << seconds << " s";
  }
  return oss.str();
}

/** Right-pad a string to at least `width` characters. */
std::string RPad(const std::string& s, size_t width)
{
  if (s.size() >= width) {
    return s;
  }
  return std::string(width - s.size(), ' ') + s;
}

} // anonymous namespace

void RenderWithCosts(const std::vector<ankerl::nanobench::Result>& results, std::ostream& out)
{
  using Measure = ankerl::nanobench::Result::Measure;

  // Header
  out << "|               ns/op |                op/s |    err% |  seeks | loaded | hashes |       "
         "   total | benchmark\n";
  out << "|--------------------:|--------------------:|--------:|-------:|-------:|-------:|-------"
         "--------:|:----------\n";

  for (const auto& r : results) {
    double ns_per_op = r.median(Measure::elapsed) * 1e9;
    double ops_per_sec = ns_per_op > 0.0 ? 1e9 / ns_per_op : 0.0;
    double err_pct = r.medianAbsolutePercentError(Measure::elapsed) * 100.0;
    double total_sec = r.sumProduct(Measure::iterations, Measure::elapsed);

    const std::string& seeks = r.context("seeks");
    const std::string& loaded = r.context("loaded");
    const std::string& hashes = r.context("hashes");

    out << "| " << RPad(FormatComma(ns_per_op, 2), 19) << " ";
    out << "| " << RPad(FormatComma(ops_per_sec, 1), 19) << " ";

    {
      std::ostringstream pct;
      pct << std::fixed << std::setprecision(1) << err_pct << "%";
      out << "| " << RPad(pct.str(), 7) << " ";
    }

    out << "| " << RPad(seeks, 6) << " ";
    out << "| " << RPad(loaded, 6) << " ";
    out << "| " << RPad(hashes, 6) << " ";
    out << "| " << RPad(FormatDuration(total_sec), 14) << " ";
    out << "| `" << r.config().mBenchmarkName << "`\n";
  }
}

// ---------------------------------------------------------------------------
// Bench utilities
// ---------------------------------------------------------------------------

namespace bench_util {
Bytes MakeKey(uint64_t i, size_t len)
{
  Bytes key(len, 0);
  // Store i in big-endian in the first 8 bytes (or fewer if len < 8).
  size_t n = std::min(len, size_t{8});
  for (size_t j = 0; j < n; ++j) {
    key[n - 1 - j] = static_cast<uint8_t>(i & 0xff);
    i >>= 8;
  }
  return key;
}

Bytes MakeValue(uint64_t i, size_t len)
{
  Bytes value(len, 0);
  // Fill with a repeating pattern seeded from i.
  for (size_t j = 0; j < len; ++j) {
    value[j] = static_cast<uint8_t>((i + j) & 0xff);
  }
  return value;
}

void PopulateDb(Db& db, size_t n, size_t key_len, size_t val_len)
{
  Path path{};
  for (size_t i = 0; i < n; ++i) {
    auto key = MakeKey(i, key_len);
    auto val = MakeValue(i, val_len);
    auto elem = Element::Item(val);
    if (!elem.has_value()) {
      continue;
    }
    auto r = db.Put(path, key, *elem);
    (void)r;
  }
}

} // namespace bench_util
} // namespace grovedb

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char** argv)
{
  benchmark::Args args;

  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--list") == 0) {
      args.is_list_only = true;
    } else if (std::strcmp(argv[i], "--filter") == 0 && i + 1 < argc) {
      args.regex_filter = argv[++i];
    } else if (std::strncmp(argv[i], "--filter=", 9) == 0) {
      args.regex_filter = argv[i] + 9;
    }
  }

  benchmark::BenchRunner::RunAll(args);
  return 0;
}

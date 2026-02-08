// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <grovedb/element.h>
#include <grovedb/types.h>

#include <bench/bench.h>

namespace {
void ElementItem(ankerl::nanobench::Bench& bench)
{
  grovedb::Bytes value(100, 0x42);
  grovedb::ClearCostContext(bench);

  bench.run("ElementItem", [&] {
    auto r = grovedb::Element::Item(value);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

void ElementEmptyTree(ankerl::nanobench::Bench& bench)
{
  grovedb::ClearCostContext(bench);

  bench.run("ElementEmptyTree", [&] {
    auto r = grovedb::Element::EmptyTree();
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

void ElementEmptySumTree(ankerl::nanobench::Bench& bench)
{
  grovedb::ClearCostContext(bench);

  bench.run("ElementEmptySumTree", [&] {
    auto r = grovedb::Element::EmptySumTree();
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

void ElementSumItem(ankerl::nanobench::Bench& bench)
{
  grovedb::ClearCostContext(bench);

  bench.run("ElementSumItem", [&] {
    auto r = grovedb::Element::SumItem(42);
    ankerl::nanobench::doNotOptimizeAway(r);
  });
}

} // anonymous namespace

BENCHMARK(ElementItem);
BENCHMARK(ElementEmptyTree);
BENCHMARK(ElementEmptySumTree);
BENCHMARK(ElementSumItem);

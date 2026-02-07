// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <FuzzedDataProvider.h>

#include <grovedb/db.h>
#include <grovedb/element.h>
#include <grovedb/query.h>
#include <grovedb/types.h>

#include <fuzz/utils/check.h>
#include <fuzz/utils/grovedb.h>
#include <fuzz/utils/tempdir.h>

#include <cstdint>
#include <vector>

namespace {
/// Maximum number of query items to generate.
static constexpr uint8_t MAX_QUERY_ITEMS = 6;

/**
 * Consume a random QueryItem from fuzz input.
 *
 * Generates one of the 10 QueryItem kinds with fuzzed operands.
 */
grovedb::QueryItem ConsumeQueryItem(FuzzedDataProvider& fdp)
{
  auto kind = fdp.ConsumeIntegralInRange<uint8_t>(0, 9);
  switch (kind) {
  case 0:
    return grovedb::QueryItem::Key(grovedb::fuzz::ConsumeKey(fdp));
  case 1:
    return grovedb::QueryItem::Range(
        grovedb::fuzz::ConsumeKey(fdp), grovedb::fuzz::ConsumeKey(fdp)
    );
  case 2:
    return grovedb::QueryItem::RangeInclusive(
        grovedb::fuzz::ConsumeKey(fdp), grovedb::fuzz::ConsumeKey(fdp)
    );
  case 3:
    return grovedb::QueryItem::RangeFull();
  case 4:
    return grovedb::QueryItem::RangeFrom(grovedb::fuzz::ConsumeKey(fdp));
  case 5:
    return grovedb::QueryItem::RangeTo(grovedb::fuzz::ConsumeKey(fdp));
  case 6:
    return grovedb::QueryItem::RangeToInclusive(grovedb::fuzz::ConsumeKey(fdp));
  case 7:
    return grovedb::QueryItem::RangeAfter(grovedb::fuzz::ConsumeKey(fdp));
  case 8:
    return grovedb::QueryItem::RangeAfterTo(
        grovedb::fuzz::ConsumeKey(fdp), grovedb::fuzz::ConsumeKey(fdp)
    );
  case 9:
    return grovedb::QueryItem::RangeAfterToInclusive(
        grovedb::fuzz::ConsumeKey(fdp), grovedb::fuzz::ConsumeKey(fdp)
    );
  default:
    return grovedb::QueryItem::RangeFull();
  }
}

/**
 * Consume a vector of random QueryItems.
 */
std::vector<grovedb::QueryItem> ConsumeQueryItems(FuzzedDataProvider& fdp)
{
  auto count = fdp.ConsumeIntegralInRange<uint8_t>(1, MAX_QUERY_ITEMS);
  std::vector<grovedb::QueryItem> items;
  items.reserve(count);
  for (uint8_t i = 0; i < count && fdp.remaining_bytes() > 2; ++i) {
    items.push_back(ConsumeQueryItem(fdp));
  }
  return items;
}
} // anonymous namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  FuzzedDataProvider fdp(data, size);

  grovedb::fuzz::TempDir dir("fuzz_query");
  auto db_result = grovedb::Db::Open(dir.path());
  if (!db_result.has_value()) {
    return 0;
  }
  auto& db = *db_result;

  grovedb::Path root_path{};

  // Create a subtree and populate with items for querying.
  grovedb::Bytes sub_key{'q'};
  {
    auto tree = grovedb::Element::EmptyTree();
    if (!tree.has_value()) {
      return 0;
    }
    auto put = db.Put(root_path, sub_key, *tree);
    if (!put.has_value()) {
      return 0;
    }
    CheckCostSanity(*put);
  }

  grovedb::Path sub_path{sub_key};

  // Insert fuzzed items into the subtree.
  auto insert_count = fdp.ConsumeIntegralInRange<uint8_t>(1, 10);
  for (uint8_t i = 0; i < insert_count && fdp.remaining_bytes() > 4; ++i) {
    auto key = grovedb::fuzz::ConsumeKey(fdp);
    if (key.empty()) {
      continue;
    }
    auto val = grovedb::fuzz::ConsumeValue(fdp);
    auto elem = grovedb::Element::Item(val);
    if (!elem.has_value()) {
      continue;
    }
    (void)db.Put(sub_path, key, *elem).transform(CheckCostSanity);
  }

  // Run query operations with fuzzed parameters.
  auto run_count = fdp.ConsumeIntegralInRange<uint8_t>(1, 8);
  for (uint8_t i = 0; i < run_count && fdp.remaining_bytes() > 4; ++i) {
    auto items = ConsumeQueryItems(fdp);
    if (items.empty()) {
      continue;
    }

    auto limit = fdp.ConsumeIntegralInRange<uint32_t>(0, 100);
    auto offset = fdp.ConsumeIntegralInRange<uint32_t>(0, 50);

    auto pq = grovedb::PathQuery::New(sub_path, items, limit, offset);
    if (!pq.has_value()) {
      continue;
    }

    auto op = fdp.ConsumeIntegralInRange<uint8_t>(0, 3);
    switch (op) {
    case 0: {
      // QueryValues
      auto r = db.QueryValues(*pq);
      if (r.has_value()) {
        CHECK_TRUE(r->value().m_data.size() <= (limit > 0 ? limit : 0xFFFFFFFF));
        CheckCostSanity(r->cost());
      }
      break;
    }
    case 1: {
      // QueryItemsOrSums
      (void)db.QueryItemsOrSums(*pq).transform(CheckCost);
      break;
    }
    case 2: {
      // QueryRaw with result_type 0 (Element)
      auto result_type = fdp.ConsumeIntegralInRange<uint8_t>(0, 2);
      (void)db.QueryRaw(*pq, result_type).transform(CheckCost);
      break;
    }
    case 3: {
      // QueryKeysOptional
      (void)db.QueryKeysOptional(*pq).transform(CheckCost);
      break;
    }
    } // switch
  }

  // Also exercise QueryManyRaw with fuzzed specs.
  if (fdp.remaining_bytes() > 8) {
    auto items1 = ConsumeQueryItems(fdp);
    auto items2 = ConsumeQueryItems(fdp);

    std::vector<grovedb::Db::RawQuerySpec> specs;
    specs.push_back({sub_path, items1, /*limit=*/10, /*offset=*/0});
    if (!items2.empty()) {
      specs.push_back({sub_path, items2, /*limit=*/5, /*offset=*/0});
    }

    auto result_type = fdp.ConsumeIntegralInRange<uint8_t>(0, 2);
    (void)db.QueryManyRaw(specs, result_type).transform(CheckCost);
  }

  return 0;
}

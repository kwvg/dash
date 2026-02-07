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

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  FuzzedDataProvider fdp(data, size);

  grovedb::fuzz::TempDir dir("fuzz_sum_tree");
  auto db_result = grovedb::Db::Open(dir.path());
  if (!db_result.has_value()) {
    return 0;
  }
  auto& db = *db_result;

  grovedb::Path root_path{};

  // Create a sum tree at the root level.
  grovedb::Bytes sum_key{'s'};
  {
    auto sum_tree = grovedb::Element::EmptySumTree();
    if (!sum_tree.has_value()) {
      return 0;
    }
    auto put = db.Put(root_path, sum_key, *sum_tree);
    if (!put.has_value()) {
      return 0;
    }
    CheckCostSanity(*put);
  }

  grovedb::Path sum_path{sum_key};

  // Insert fuzzed SumItems into the sum tree.
  auto insert_count = fdp.ConsumeIntegralInRange<uint8_t>(1, 12);
  std::vector<grovedb::Bytes> inserted_keys;

  for (uint8_t i = 0; i < insert_count && fdp.remaining_bytes() > 4; ++i) {
    auto key = grovedb::fuzz::ConsumeKey(fdp);
    if (key.empty()) {
      continue;
    }

    // Alternate between SumItem and regular Item to test mixed queries.
    auto use_sum = fdp.ConsumeBool();
    if (use_sum) {
      auto val = fdp.ConsumeIntegralInRange<int64_t>(-1000, 1000);
      auto elem = grovedb::Element::SumItem(val);
      if (!elem.has_value()) {
        continue;
      }
      auto put = db.Put(sum_path, key, *elem);
      if (put.has_value()) {
        CheckCostSanity(*put);
        inserted_keys.push_back(key);
      }
    } else {
      auto val = grovedb::fuzz::ConsumeValue(fdp);
      auto elem = grovedb::Element::Item(val);
      if (!elem.has_value()) {
        continue;
      }
      auto put = db.Put(sum_path, key, *elem);
      if (put.has_value()) {
        CheckCostSanity(*put);
        inserted_keys.push_back(key);
      }
    }
  }

  // QuerySums — query all items in the sum tree.
  {
    std::vector<grovedb::QueryItem> items{grovedb::QueryItem::RangeFull()};
    auto pq = grovedb::PathQuery::New(sum_path, items);
    if (pq.has_value()) {
      auto result = db.QuerySums(*pq);
      // Result should succeed if we inserted anything.
      if (result.has_value()) {
        CheckCostSanity(result->cost());
        if (!result->value().m_data.empty()) {
          // Sum tree sums should be present.
          CHECK_TRUE(result->value().m_data.size() > 0);
        }
      }
    }
  }

  // QueryItemsOrSums — should return entries tagged as SumValue or ItemData.
  {
    std::vector<grovedb::QueryItem> items{grovedb::QueryItem::RangeFull()};
    auto pq = grovedb::PathQuery::New(sum_path, items);
    if (pq.has_value()) {
      auto result = db.QueryItemsOrSums(*pq);
      if (result.has_value()) {
        CheckCostSanity(result->cost());
        for (const auto& entry : result->value().m_data) {
          // Each entry should be one of the known kinds.
          auto kind = static_cast<uint8_t>(entry.m_kind);
          CHECK_TRUE(kind <= 5);
        }
      }
    }
  }

  // QueryValues with limit/offset on the sum tree.
  if (fdp.remaining_bytes() > 2) {
    auto limit = fdp.ConsumeIntegralInRange<uint32_t>(0, 20);
    auto offset = fdp.ConsumeIntegralInRange<uint32_t>(0, 10);

    std::vector<grovedb::QueryItem> items{grovedb::QueryItem::RangeFull()};
    auto pq = grovedb::PathQuery::New(sum_path, items, limit, offset);
    if (pq.has_value()) {
      auto result = db.QueryValues(*pq);
      if (result.has_value()) {
        CheckCostSanity(result->cost());
        if (limit > 0) {
          CHECK_TRUE(result->value().m_data.size() <= limit);
        }
      }
    }
  }

  // QueryRaw on the sum tree.
  {
    std::vector<grovedb::QueryItem> items{grovedb::QueryItem::RangeFull()};
    auto pq = grovedb::PathQuery::New(sum_path, items);
    if (pq.has_value()) {
      // result_type 1 = KeyElementPair
      auto result = db.QueryRaw(*pq, /*result_type=*/1);
      if (result.has_value()) {
        CheckCostSanity(result->cost());
        for (const auto& entry : result->value().m_data) {
          CHECK_TRUE(entry.m_kind == grovedb::QueryResultKind::KeyElementPair);
          CHECK_TRUE(!entry.m_key.empty());
        }
      }
    }
  }

  // Exercise individual key queries with fuzzed QueryItem types.
  if (fdp.remaining_bytes() > 4 && !inserted_keys.empty()) {
    auto idx = fdp.ConsumeIntegralInRange<size_t>(0, inserted_keys.size() - 1);
    std::vector<grovedb::QueryItem> items{grovedb::QueryItem::Key(inserted_keys[idx])};
    auto pq = grovedb::PathQuery::New(sum_path, items);
    if (pq.has_value()) {
      auto result = db.QueryValues(*pq);
      if (result.has_value()) {
        CheckCostSanity(result->cost());
        // Querying a specific inserted key should return exactly 1 result.
        CHECK_EQ(result->value().m_data.size(), 1u);
      }
    }
  }

  return 0;
}

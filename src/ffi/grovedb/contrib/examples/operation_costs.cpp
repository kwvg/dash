// Copyright (c) 2026-present, The Dash Core developers
// Copyright (c) 2017-present, Dash Core Group, Inc.
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit
//
// Adapted from drive-sdk's cost tracking patterns

#include <test/util/tempdir.h>

#include <grovedb/db.h>
#include <grovedb/element.h>

#include <util/logging.h>

#include <cstdlib>
#include <format>
#include <string>

namespace {
/** Log all 6 OperationCost fields. */
void LogCost(const char* name, const grovedb::OperationCost& cost)
{
  Log(INFO,
      "{}: seeks={}, added={}, replaced={}, removed={}, loaded={}, hashes={}",
      name,
      cost.m_seek_count,
      cost.m_storage_added_bytes,
      cost.m_storage_replaced_bytes,
      cost.m_storage_removed_bytes,
      cost.m_storage_loaded_bytes,
      cost.m_hash_node_calls);
}
} // anonymous namespace

int main()
{
  Log(INFO, "=== Operation Costs ===");

  grovedb::test::TempDir dir("ex_op_costs");
  auto result = grovedb::Db::Open(dir.PathToString());
  if (!result.has_value()) {
    Log(ERROR, "failed to open database: {}", result.error().message());
    return EXIT_FAILURE;
  }
  auto db = std::move(result).value();
  Log(INFO, "database opened at {}", dir.PathToString());

  grovedb::Path root{};

  // -------------------------------------------------------------------------
  // 1. Put — log all cost fields.
  // -------------------------------------------------------------------------

  Log(INFO, "--- Put cost ---");
  auto put = grovedb::Element::Item(grovedb::Bytes::FromString("cost_tracking"))
                 .and_then([&](grovedb::Element e) {
                   return db.Put(root, grovedb::Bytes::FromString("key1"), e);
                 });
  if (!put.has_value()) {
    Log(ERROR, "put failed: {}", put.error().message());
    return EXIT_FAILURE;
  }
  LogCost("Put('key1')", *put);

  // -------------------------------------------------------------------------
  // 2. Get — Costed<Element>, show .value() and .cost() access.
  // -------------------------------------------------------------------------

  Log(INFO, "--- Get cost (Costed<Element>) ---");
  auto get = db.Get(root, grovedb::Bytes::FromString("key1"));
  if (!get.has_value()) {
    Log(ERROR, "get failed: {}", get.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "Get returned element with {} raw bytes", get->value().data().size());
  LogCost("Get('key1')", get->cost());

  // -------------------------------------------------------------------------
  // 3. Insert 10 items and accumulate costs.
  // -------------------------------------------------------------------------

  Log(INFO, "--- cumulative cost over 10 inserts ---");
  grovedb::OperationCost cumulative{};
  for (int i = 0; i < 10; ++i) {
    auto key = grovedb::Bytes::FromString(std::format("batch_{:03d}", i));
    auto val = grovedb::Bytes::FromString(std::format("value_{}", i));
    auto cost = grovedb::Element::Item(val).and_then([&](grovedb::Element e) {
      return db.Put(root, key, e);
    });
    if (!cost.has_value()) {
      Log(ERROR, "put failed for batch_{:03d}: {}", i, cost.error().message());
      return EXIT_FAILURE;
    }
    cumulative += *cost;
  }
  LogCost("cumulative (10 puts)", cumulative);

  // -------------------------------------------------------------------------
  // 4. Commit a transaction — log commit cost.
  // -------------------------------------------------------------------------

  Log(INFO, "--- transaction commit cost ---");
  auto txn = db.BeginTransaction();
  if (!txn.has_value()) {
    Log(ERROR, "BeginTransaction failed: {}", txn.error().message());
    return EXIT_FAILURE;
  }

  auto tp = grovedb::Element::Item(grovedb::Bytes::FromString("txn_data"))
                .and_then([&](grovedb::Element e) {
                  return db.Put(root, grovedb::Bytes::FromString("txn_key"), e, *txn);
                });
  if (!tp.has_value()) {
    Log(ERROR, "put in txn failed: {}", tp.error().message());
    return EXIT_FAILURE;
  }
  LogCost("Put (in txn)", *tp);

  auto commit = db.Commit(*txn);
  if (!commit.has_value()) {
    Log(ERROR, "commit failed: {}", commit.error().message());
    return EXIT_FAILURE;
  }
  LogCost("Commit", *commit);

  // -------------------------------------------------------------------------
  // 5. Compare Get vs KeyExists vs GetDirect costs side-by-side.
  // -------------------------------------------------------------------------

  Log(INFO, "--- cost comparison: Get vs KeyExists vs GetDirect ---");

  auto c_get = db.Get(root, grovedb::Bytes::FromString("key1"));
  if (c_get.has_value()) {
    LogCost("Get", c_get->cost());
  }

  auto c_exists = db.KeyExists(root, grovedb::Bytes::FromString("key1"));
  if (c_exists.has_value()) {
    LogCost("KeyExists", c_exists->cost());
  }

  auto c_direct = db.GetDirect(root, grovedb::Bytes::FromString("key1"));
  if (c_direct.has_value()) {
    LogCost("GetDirect", c_direct->cost());
  }

  Log(INFO, "done");
  return EXIT_SUCCESS;
}

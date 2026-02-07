// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <FuzzedDataProvider.h>

#include <grovedb/db.h>
#include <grovedb/element.h>
#include <grovedb/types.h>

#include <fuzz/utils/check.h>
#include <fuzz/utils/grovedb.h>
#include <fuzz/utils/tempdir.h>

#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  FuzzedDataProvider fdp(data, size);

  grovedb::fuzz::TempDir dir("fuzz_transaction");
  auto db_result = grovedb::Db::Open(dir.path());
  if (!db_result.has_value()) {
    return 0;
  }
  auto& db = *db_result;

  grovedb::Path root_path{};
  bool should_commit = fdp.ConsumeBool();

  // Begin a transaction.
  auto tx_result = db.BeginTransaction();
  CHECK_OK(tx_result);
  auto& tx = *tx_result;

  // Insert items within the transaction.
  auto count = fdp.ConsumeIntegralInRange<uint8_t>(1, 8);
  grovedb::Bytes last_key;
  bool any_inserted = false;

  for (uint8_t i = 0; i < count && fdp.remaining_bytes() > 2; ++i) {
    auto key = grovedb::fuzz::ConsumeKey(fdp);
    if (key.empty()) {
      continue;
    }

    auto elem = grovedb::fuzz::ConsumeElement(fdp);
    if (elem.empty()) {
      continue;
    }

    auto put_result = db.Put(root_path, key, elem, tx);
    if (put_result.has_value()) {
      CheckCostSanity(*put_result);
      last_key = key;
      any_inserted = true;

      // Verify key is visible within the transaction.
      auto exists = db.KeyExists(root_path, key, tx);
      CHECK_OK(exists);
      CHECK_TRUE(exists->value());
      CheckCostSanity(exists->cost());
    }
  }

  if (should_commit) {
    auto cost = db.Commit(tx);
    CHECK_OK(cost);
    CheckCostSanity(*cost);

    // After commit, inserted keys should be visible outside the transaction.
    if (any_inserted && !last_key.empty()) {
      auto exists = db.KeyExists(root_path, last_key);
      CHECK_OK(exists);
      CHECK_TRUE(exists->value());
      CheckCostSanity(exists->cost());
    }
  } else {
    auto result = db.Rollback(tx);
    CHECK_OK(result);

    // After rollback, inserted keys should NOT be visible.
    if (any_inserted && !last_key.empty()) {
      auto exists = db.KeyExists(root_path, last_key);
      CHECK_OK(exists);
      CHECK_TRUE(!exists->value());
      CheckCostSanity(exists->cost());
    }
  }

  return 0;
}

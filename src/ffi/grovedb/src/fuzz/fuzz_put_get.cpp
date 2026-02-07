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

  grovedb::fuzz::TempDir dir("fuzz_put_get");
  auto db_result = grovedb::Db::Open(dir.path());
  if (!db_result.has_value()) {
    return 0;
  }
  auto& db = *db_result;

  grovedb::Path root_path{};
  auto ops = fdp.ConsumeIntegralInRange<uint8_t>(1, 16);

  for (uint8_t i = 0; i < ops && fdp.remaining_bytes() > 2; ++i) {
    auto op = fdp.ConsumeIntegralInRange<uint8_t>(0, 4);
    auto key = grovedb::fuzz::ConsumeKey(fdp);
    if (key.empty()) {
      continue;
    }

    switch (op) {
    case 0: { // Put
      auto elem = grovedb::fuzz::ConsumeElement(fdp);
      if (!elem.empty()) {
        (void)db.Put(root_path, key, elem).transform(CheckCostSanity);
      }
      break;
    }
    case 1: { // Get
      auto result = db.Get(root_path, key);
      if (result.has_value()) {
        CHECK_TRUE(!result->value().empty());
        CheckCostSanity(result->cost());
      }
      break;
    }
    case 2: { // GetDirect
      auto result = db.GetDirect(root_path, key);
      if (result.has_value()) {
        CHECK_TRUE(!result->value().empty());
        CheckCostSanity(result->cost());
      }
      break;
    }
    case 3: { // GetOptional
      auto result = db.GetOptional(root_path, key);
      if (result.has_value()) {
        // may or may not have a value — that's fine
        if (result->value().has_value()) {
          CHECK_TRUE(!result->value()->empty());
        }
        CheckCostSanity(result->cost());
      }
      break;
    }
    case 4: { // KeyExists
      (void)db.KeyExists(root_path, key).transform(CheckCost);
      break;
    }
    default:
      break;
    }
  }

  return 0;
}

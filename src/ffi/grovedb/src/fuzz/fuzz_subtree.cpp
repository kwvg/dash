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

  grovedb::fuzz::TempDir dir("fuzz_subtree");
  auto db_result = grovedb::Db::Open(dir.path());
  if (!db_result.has_value()) {
    return 0;
  }
  auto& db = *db_result;

  grovedb::Path root_path{};

  // Root should always exist and be empty initially.
  {
    auto exists = db.SubtreeExists(root_path);
    CHECK_OK(exists);
    CHECK_TRUE(exists->value());
    CheckCostSanity(exists->cost());

    auto empty = db.IsEmptyTree(root_path);
    CHECK_OK(empty);
    CHECK_TRUE(empty->value());
    CheckCostSanity(empty->cost());
  }

  // Create subtrees with fuzzed names at the root level.
  auto count = fdp.ConsumeIntegralInRange<uint8_t>(1, 6);
  for (uint8_t i = 0; i < count && fdp.remaining_bytes() > 2; ++i) {
    auto key = grovedb::fuzz::ConsumeKey(fdp);
    if (key.empty()) {
      continue;
    }

    auto tree_elem = grovedb::Element::EmptyTree();
    if (!tree_elem.has_value()) {
      continue;
    }

    auto put_result = db.Put(root_path, key, *tree_elem);
    if (!put_result.has_value()) {
      continue;
    }
    CheckCostSanity(*put_result);

    // Subtree path should now exist.
    grovedb::Path subtree_path{key};
    auto exists = db.SubtreeExists(subtree_path);
    CHECK_OK(exists);
    CHECK_TRUE(exists->value());
    CheckCostSanity(exists->cost());

    // Newly created subtree should be empty.
    auto empty = db.IsEmptyTree(subtree_path);
    CHECK_OK(empty);
    CHECK_TRUE(empty->value());
    CheckCostSanity(empty->cost());

    // Insert an item into the subtree.
    auto item_key = grovedb::fuzz::ConsumeKey(fdp);
    if (item_key.empty()) {
      continue;
    }

    auto item_elem = grovedb::fuzz::ConsumeElement(fdp);
    if (item_elem.empty()) {
      continue;
    }

    auto insert_result = db.Put(subtree_path, item_key, item_elem);
    if (!insert_result.has_value()) {
      continue;
    }
    CheckCostSanity(*insert_result);

    // Subtree should no longer be empty.
    auto not_empty = db.IsEmptyTree(subtree_path);
    CHECK_OK(not_empty);
    CHECK_TRUE(!not_empty->value());
    CheckCostSanity(not_empty->cost());
  }

  // Root should no longer be empty if anything was inserted.
  {
    (void)db.IsEmptyTree(root_path).transform(CheckCost);
  }

  // Check a non-existent path.
  {
    auto key = grovedb::fuzz::ConsumeKey(fdp);
    grovedb::Path missing{grovedb::Bytes::FromString("_no"), key};
    auto exists = db.SubtreeExists(missing);
    CHECK_OK(exists);
    CHECK_TRUE(!exists->value());
    CheckCostSanity(exists->cost());
  }

  return 0;
}

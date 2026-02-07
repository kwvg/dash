// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <FuzzedDataProvider.h>

#include <grovedb/batch.h>
#include <grovedb/db.h>
#include <grovedb/element.h>
#include <grovedb/types.h>

#include <fuzz/utils/check.h>
#include <fuzz/utils/grovedb.h>
#include <fuzz/utils/tempdir.h>

#include <cstdint>
#include <vector>

namespace {
/// Maximum number of operations in a single batch.
static constexpr uint8_t MAX_BATCH_OPS = 8;

/**
 * Consume a random batch operation from fuzz input.
 */
grovedb::BatchOperation ConsumeBatchOp(FuzzedDataProvider& fdp, const grovedb::Path& path)
{
  auto kind = fdp.ConsumeIntegralInRange<uint8_t>(0, 4);
  auto key = grovedb::fuzz::ConsumeKey(fdp);

  switch (kind) {
  case 0: {
    // InsertOnly
    auto val = grovedb::fuzz::ConsumeValue(fdp);
    auto elem = grovedb::Element::Item(val);
    if (!elem.has_value()) {
      return grovedb::BatchOperation::Delete(path, key);
    }
    return grovedb::BatchOperation::InsertOnly(path, key, *elem);
  }
  case 1: {
    // InsertOrReplace
    auto val = grovedb::fuzz::ConsumeValue(fdp);
    auto elem = grovedb::Element::Item(val);
    if (!elem.has_value()) {
      return grovedb::BatchOperation::Delete(path, key);
    }
    return grovedb::BatchOperation::InsertOrReplace(path, key, *elem);
  }
  case 2: {
    // Replace
    auto val = grovedb::fuzz::ConsumeValue(fdp);
    auto elem = grovedb::Element::Item(val);
    if (!elem.has_value()) {
      return grovedb::BatchOperation::Delete(path, key);
    }
    return grovedb::BatchOperation::Replace(path, key, *elem);
  }
  case 3: {
    // Delete
    return grovedb::BatchOperation::Delete(path, key);
  }
  case 4: {
    // DeleteTree
    return grovedb::BatchOperation::DeleteTree(path, key);
  }
  default:
    return grovedb::BatchOperation::Delete(path, key);
  }
}
} // anonymous namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  FuzzedDataProvider fdp(data, size);

  grovedb::fuzz::TempDir dir("fuzz_batch");
  auto db_result = grovedb::Db::Open(dir.path());
  if (!db_result.has_value()) {
    return 0;
  }
  auto& db = *db_result;

  grovedb::Path root_path{};

  // Pre-populate some items so replace/delete have targets.
  auto seed_count = fdp.ConsumeIntegralInRange<uint8_t>(0, 5);
  for (uint8_t i = 0; i < seed_count && fdp.remaining_bytes() > 4; ++i) {
    auto key = grovedb::fuzz::ConsumeKey(fdp);
    if (key.empty()) {
      continue;
    }
    auto val = grovedb::fuzz::ConsumeValue(fdp);
    auto elem = grovedb::Element::Item(val);
    if (!elem.has_value()) {
      continue;
    }
    (void)db.Put(root_path, key, *elem).transform(CheckCostSanity);
  }

  // Build and apply batches.
  auto batch_count = fdp.ConsumeIntegralInRange<uint8_t>(1, 3);
  for (uint8_t b = 0; b < batch_count && fdp.remaining_bytes() > 4; ++b) {
    auto op_count = fdp.ConsumeIntegralInRange<uint8_t>(1, MAX_BATCH_OPS);
    std::vector<grovedb::BatchOperation> ops;
    ops.reserve(op_count);
    for (uint8_t i = 0; i < op_count && fdp.remaining_bytes() > 4; ++i) {
      ops.push_back(ConsumeBatchOp(fdp, root_path));
    }
    if (ops.empty()) {
      continue;
    }

    grovedb::BatchApplyOptions options;
    options.m_allow_deleting_non_empty_trees = fdp.ConsumeBool();
    options.m_disable_operation_consistency_check = fdp.ConsumeBool();

    // Apply — may succeed or fail depending on fuzz data (that's OK).
    (void)db.ApplyBatch(ops, options).transform(CheckCostSanity);
  }

  return 0;
}

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

/// Instruction opcodes for the tree-state program.
enum class Op : uint8_t {
  SetItem = 0,
  RemoveItem,
  CreateSubtree,
  DeleteIfEmptyTree,
  ClearSubtree,
  VerifyIntegrity,
};

static constexpr uint8_t OP_COUNT = 6;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  FuzzedDataProvider fdp(data, size);

  grovedb::fuzz::TempDir dir("fuzz_tree_state");
  auto db_result = grovedb::Db::Open(dir.path());
  if (!db_result.has_value()) {
    return 0;
  }
  auto& db = *db_result;

  grovedb::Path root_path{};

  // Pre-create a few subtrees so operations have targets.
  grovedb::Bytes sub_a{'a'};
  grovedb::Bytes sub_b{'b'};
  {
    auto tree = grovedb::Element::EmptyTree();
    if (tree.has_value()) {
      (void)db.Put(root_path, sub_a, *tree).transform(CheckCostSanity);
      (void)db.Put(root_path, sub_b, *tree).transform(CheckCostSanity);
    }
  }

  auto instruction_count = fdp.ConsumeIntegralInRange<uint8_t>(1, 20);

  for (uint8_t i = 0; i < instruction_count && fdp.remaining_bytes() > 2; ++i) {
    auto op = static_cast<Op>(fdp.ConsumeIntegralInRange<uint8_t>(0, OP_COUNT - 1));

    // Pick a subtree for operations: 0=root, 1=sub_a, 2=sub_b
    auto target = fdp.ConsumeIntegralInRange<uint8_t>(0, 2);
    grovedb::Path path;
    if (target == 1) {
      path = {sub_a};
    } else if (target == 2) {
      path = {sub_b};
    }

    switch (op) {
    case Op::SetItem: {
      auto key = grovedb::fuzz::ConsumeKey(fdp);
      if (key.empty()) {
        break;
      }
      auto elem = grovedb::fuzz::ConsumeElement(fdp);
      if (elem.empty()) {
        break;
      }
      (void)db.Put(path, key, elem).transform(CheckCostSanity);
      break;
    }
    case Op::RemoveItem: {
      auto key = grovedb::fuzz::ConsumeKey(fdp);
      if (key.empty()) {
        break;
      }
      (void)db.Delete(path, key).transform(CheckCostSanity);
      break;
    }
    case Op::CreateSubtree: {
      auto key = grovedb::fuzz::ConsumeKey(fdp);
      if (key.empty()) {
        break;
      }
      auto tree = grovedb::Element::EmptyTree();
      if (tree.has_value()) {
        (void)db.Put(path, key, *tree).transform(CheckCostSanity);
      }
      break;
    }
    case Op::DeleteIfEmptyTree: {
      auto key = grovedb::fuzz::ConsumeKey(fdp);
      if (key.empty()) {
        break;
      }
      (void)db.DeleteIfEmpty(path, key).transform(CheckCost);
      break;
    }
    case Op::ClearSubtree: {
      (void)db.Clear(path);
      break;
    }
    case Op::VerifyIntegrity: {
      auto result = db.VerifyIntegrity();
      // If verification succeeds, tree should be valid.
      if (result.has_value()) {
        CHECK_TRUE(*result);
      }
      break;
    }
    } // switch
  }

  // Final integrity check.
  {
    auto result = db.VerifyIntegrity();
    if (result.has_value()) {
      CHECK_TRUE(*result);
    }
  }

  return 0;
}

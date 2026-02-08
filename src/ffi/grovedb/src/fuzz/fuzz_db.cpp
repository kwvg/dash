// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <FuzzedDataProvider.h>

#include <grovedb/batch.h>
#include <grovedb/db.h>
#include <grovedb/element.h>
#include <grovedb/query.h>
#include <grovedb/types.h>

#include <fuzz/utils/check.h>
#include <fuzz/utils/grovedb.h>
#include <fuzz/utils/tempdir.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {
/// Number of distinct operation types.
static constexpr uint8_t OP_COUNT = 12;

/// Maximum batch operations per apply.
static constexpr uint8_t MAX_BATCH_OPS = 4;
} // anonymous namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  FuzzedDataProvider fdp(data, size);

  grovedb::fuzz::TempDir dir("fuzz_db");
  auto db_result = grovedb::Db::Open(dir.path());
  if (!db_result.has_value()) {
    return 0;
  }
  auto& db = *db_result;

  grovedb::Path root_path{};
  auto iterations = fdp.ConsumeIntegralInRange<uint8_t>(1, 20);

  for (uint8_t iter = 0; iter < iterations && fdp.remaining_bytes() > 4; ++iter) {
    auto op = fdp.ConsumeIntegralInRange<uint8_t>(0, OP_COUNT - 1);

    switch (op) {
    case 0: { // Put
      auto key = grovedb::fuzz::ConsumeKey(fdp);
      if (key.empty()) {
        break;
      }
      auto elem = grovedb::fuzz::ConsumeElement(fdp);
      if (elem.empty()) {
        break;
      }
      (void)db.Put(root_path, key, elem).transform(CheckCostSanity);
      break;
    }
    case 1: { // Get
      auto key = grovedb::fuzz::ConsumeKey(fdp);
      if (key.empty()) {
        break;
      }
      auto result = db.Get(root_path, key);
      if (result.has_value()) {
        CHECK_TRUE(!result->value().empty());
        CheckCostSanity(result->cost());
      }
      break;
    }
    case 2: { // Delete
      auto key = grovedb::fuzz::ConsumeKey(fdp);
      if (key.empty()) {
        break;
      }
      (void)db.Delete(root_path, key).transform(CheckCostSanity);
      break;
    }
    case 3: { // KeyExists
      auto key = grovedb::fuzz::ConsumeKey(fdp);
      if (key.empty()) {
        break;
      }
      (void)db.KeyExists(root_path, key).transform(CheckCost);
      break;
    }
    case 4: { // Transaction: put + commit/rollback
      auto tx_result = db.BeginTransaction();
      if (!tx_result.has_value()) {
        break;
      }
      auto& tx = *tx_result;

      auto key = grovedb::fuzz::ConsumeKey(fdp);
      if (!key.empty()) {
        auto elem = grovedb::fuzz::ConsumeElement(fdp);
        if (!elem.empty()) {
          (void)db.Put(root_path, key, elem, tx).transform(CheckCostSanity);
        }
      }

      if (fdp.ConsumeBool()) {
        (void)db.Commit(tx).transform(CheckCostSanity);
      } else {
        (void)db.Rollback(tx);
      }
      break;
    }
    case 5: { // Batch apply
      auto op_count = fdp.ConsumeIntegralInRange<uint8_t>(1, MAX_BATCH_OPS);
      std::vector<grovedb::BatchOperation> ops;
      for (uint8_t i = 0; i < op_count && fdp.remaining_bytes() > 4; ++i) {
        auto key = grovedb::fuzz::ConsumeKey(fdp);
        if (key.empty()) {
          continue;
        }
        auto val = grovedb::fuzz::ConsumeValue(fdp);
        auto elem = grovedb::Element::Item(val);
        if (!elem.has_value()) {
          continue;
        }
        ops.push_back(grovedb::BatchOperation::InsertOrReplace(root_path, key, *elem));
      }
      if (!ops.empty()) {
        grovedb::BatchApplyOptions options{};
        (void)db.ApplyBatch(ops, options).transform(CheckCostSanity);
      }
      break;
    }
    case 6: { // PutAux + GetAux
      auto key = grovedb::fuzz::ConsumeKey(fdp);
      if (key.empty()) {
        break;
      }
      auto val = grovedb::fuzz::ConsumeValue(fdp);
      auto put_result = db.PutAux(key, val);
      if (put_result.has_value()) {
        CheckCostSanity(*put_result);
        auto get_result = db.GetAux(key);
        if (get_result.has_value()) {
          CheckCostSanity(get_result->cost());
          if (get_result->value().has_value()) {
            CHECK_EQ(*get_result->value(), val);
          }
        }
      }
      break;
    }
    case 7: { // DeleteAux
      auto key = grovedb::fuzz::ConsumeKey(fdp);
      if (key.empty()) {
        break;
      }
      (void)db.DeleteAux(key).transform(CheckCostSanity);
      break;
    }
    case 8: { // FindSubtrees
      (void)db.FindSubtrees(root_path).transform(CheckCost);
      break;
    }
    case 9: { // Checkpoint: create + open + read
      std::string ckpt = dir.path() + "/ckpt_" + std::to_string(iter);
      auto create = db.CreateCheckpoint(ckpt);
      if (create.has_value()) {
        auto ckpt_db = grovedb::Db::OpenCheckpoint(ckpt);
        if (ckpt_db.has_value()) {
          // Just verify it opens and root hash is readable.
          (void)ckpt_db->GetRootHash().transform(CheckCost);
        }
        (void)grovedb::Db::DeleteCheckpoint(ckpt);
      }
      break;
    }
    case 10: { // CreateSubtree + Put into it
      auto subtree_key = grovedb::fuzz::ConsumeKey(fdp);
      if (subtree_key.empty()) {
        break;
      }
      auto tree = grovedb::Element::EmptyTree();
      if (!tree.has_value()) {
        break;
      }
      auto put = db.Put(root_path, subtree_key, *tree);
      if (put.has_value()) {
        CheckCostSanity(*put);
        grovedb::Path child_path{subtree_key};
        auto inner_key = grovedb::fuzz::ConsumeKey(fdp);
        if (!inner_key.empty()) {
          auto elem = grovedb::fuzz::ConsumeElement(fdp);
          if (!elem.empty()) {
            (void)db.Put(child_path, inner_key, elem).transform(CheckCostSanity);
          }
        }
      }
      break;
    }
    case 11: { // VerifyIntegrity
      auto result = db.VerifyIntegrity();
      // Should always succeed on a well-formed database.
      if (result.has_value()) {
        CHECK_TRUE(*result);
      }
      break;
    }
    default:
      break;
    }
  }

  // Final integrity check.
  auto integrity = db.VerifyIntegrity();
  if (integrity.has_value()) {
    CHECK_TRUE(*integrity);
  }

  return 0;
}

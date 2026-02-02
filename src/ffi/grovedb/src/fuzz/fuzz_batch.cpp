// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// FUZZ TARGET: fuzz_batch
// Demonstrates: BatchOperation::InsertOnly/InsertOrReplace/Delete,
//               ApplyBatch(), BatchApplyOptions
//

#include <FuzzedDataProvider.h>
#include <utils/check.h>
#include <utils/grovedb.h>
#include <utils/tempdir.h>

#include <grovedb/db.h>

#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    grovedb::fuzz::TempDir tmp{"fuzz_batch"};
    grovedb::Db db;
    if (!grovedb::Db::Open(tmp.path(), db).ok()) return 0;

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Build a small batch of InsertOnly operations.
    auto count = fdp.ConsumeIntegralInRange<size_t>(1, 8);
    std::vector<grovedb::BatchOperation> ops;
    std::vector<grovedb::Bytes> keys;

    for (size_t i{0}; i < count && fdp.remaining_bytes() > 0; ++i) {
        grovedb::Bytes key = grovedb::fuzz::ConsumeKey(fdp);
        grovedb::Bytes val = grovedb::fuzz::ConsumeValue(fdp);
        grovedb::Element item;
        if (!grovedb::Element::Item(val, item).ok()) continue;

        keys.push_back(key);
        ops.push_back(grovedb::BatchOperation::InsertOnly(root, key, item));
    }

    if (ops.empty()) return 0;

    // Apply batch with validation options.
    grovedb::BatchApplyOptions options{};
    options.m_validate_insertion_does_not_override = true;

    auto s = db.ApplyBatch(ops, options, cost);
    if (!s.ok()) return 0;

    // Verify all keys exist.
    for (const auto& key : keys) {
        bool exists{false};
        CHECK_OK(db.KeyExists(root, key, exists, cost));
        CHECK_TRUE(exists);
    }

    // Now build an InsertOrReplace batch that overwrites one key.
    if (!keys.empty() && fdp.remaining_bytes() > 0) {
        grovedb::Bytes new_val = grovedb::fuzz::ConsumeValue(fdp);
        grovedb::Element new_item;
        if (grovedb::Element::Item(new_val, new_item).ok()) {
            std::vector<grovedb::BatchOperation> replace_ops;
            replace_ops.push_back(
                grovedb::BatchOperation::InsertOrReplace(root, keys[0], new_item));
            (void)db.ApplyBatch(replace_ops, cost);
        }
    }

    return 0;
}

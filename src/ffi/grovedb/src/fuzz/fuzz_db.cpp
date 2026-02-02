// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// FUZZ TARGET: fuzz_db
// Stateful: random operation sequence.
//
// Adapted from RocksDB fuzz/db_fuzzer.cc
// Copyright (c) Meta Platforms, Inc. and affiliates.
// Licensed under GPLv2 and Apache 2.0.
//

#include <FuzzedDataProvider.h>
#include <utils/check.h>
#include <utils/grovedb.h>
#include <utils/tempdir.h>

#include <grovedb/db.h>

#include <cstdint>
#include <vector>

namespace {

enum Op : uint8_t {
    kPut,
    kGet,
    kDelete,
    kQuery,
    kBatch,
    kTransaction,
    kProve,
    kSubtree,
    OP_COUNT,
};

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    grovedb::fuzz::TempDir tmp{"fuzz_db"};
    grovedb::Db db;
    if (!grovedb::Db::Open(tmp.path(), db).ok()) return 0;

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    auto max_iter = fdp.ConsumeIntegralInRange<size_t>(1, 64);

    for (size_t iter{0}; iter < max_iter && fdp.remaining_bytes() > 0; ++iter) {
        auto op = static_cast<Op>(fdp.ConsumeIntegralInRange<uint8_t>(0, OP_COUNT - 1));

        switch (op) {
        case kPut: {
            grovedb::Bytes key = grovedb::fuzz::ConsumeKey(fdp);
            grovedb::Bytes val = grovedb::fuzz::ConsumeValue(fdp);
            grovedb::Element item;
            if (grovedb::Element::Item(val, item).ok()) {
                (void)db.Put(root, key, item, cost);
            }
            break;
        }
        case kGet: {
            grovedb::Bytes key = grovedb::fuzz::ConsumeKey(fdp);
            grovedb::Element fetched;
            (void)db.GetDirect(root, key, fetched, cost);
            break;
        }
        case kDelete: {
            grovedb::Bytes key = grovedb::fuzz::ConsumeKey(fdp);
            (void)db.Delete(root, key, cost);
            break;
        }
        case kQuery: {
            grovedb::PathQuery pq;
            auto s = grovedb::PathQuery::New(
                root, {grovedb::QueryItem::RangeFull()},
                fdp.ConsumeIntegralInRange<uint32_t>(0, 10),
                /*offset=*/0, pq);
            if (s.ok()) {
                std::vector<grovedb::Bytes> values;
                uint16_t skipped{0};
                (void)db.QueryValues(pq, values, skipped, cost);
            }
            break;
        }
        case kBatch: {
            auto count = fdp.ConsumeIntegralInRange<size_t>(1, 4);
            std::vector<grovedb::BatchOperation> ops;
            for (size_t i{0}; i < count && fdp.remaining_bytes() > 0; ++i) {
                grovedb::Bytes key = grovedb::fuzz::ConsumeKey(fdp);
                grovedb::Bytes val = grovedb::fuzz::ConsumeValue(fdp);
                grovedb::Element item;
                if (grovedb::Element::Item(val, item).ok()) {
                    ops.push_back(grovedb::BatchOperation::InsertOrReplace(root, key, item));
                }
            }
            if (!ops.empty()) {
                (void)db.ApplyBatch(ops, cost);
            }
            break;
        }
        case kTransaction: {
            grovedb::Transaction txn;
            if (!db.BeginTransaction(txn).ok()) break;

            grovedb::Bytes key = grovedb::fuzz::ConsumeKey(fdp);
            grovedb::Bytes val = grovedb::fuzz::ConsumeValue(fdp);
            grovedb::Element item;
            if (grovedb::Element::Item(val, item).ok()) {
                (void)db.Put(root, key, item, txn, cost);
            }

            if (fdp.ConsumeBool()) {
                (void)db.Commit(txn, cost);
            } else {
                (void)db.Rollback(txn);
            }
            break;
        }
        case kProve: {
            grovedb::PathQuery pq;
            auto s = grovedb::PathQuery::New(
                root, {grovedb::QueryItem::RangeFull()},
                /*limit=*/5, /*offset=*/0, pq);
            if (s.ok()) {
                grovedb::Bytes proof;
                (void)db.Prove(pq, proof, cost);
            }
            break;
        }
        case kSubtree: {
            grovedb::Bytes st_key = grovedb::fuzz::ConsumeKey(fdp);
            grovedb::Element tree;
            if (grovedb::Element::EmptyTree(tree).ok()) {
                (void)db.Put(root, st_key, tree, cost);
            }

            bool exists{false};
            (void)db.SubtreeExists(grovedb::Path{st_key}, exists, cost);
            break;
        }
        default:
            break;
        }
    }

    return 0;
}

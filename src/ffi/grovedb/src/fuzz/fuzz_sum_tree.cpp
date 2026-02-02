// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// FUZZ TARGET: fuzz_sum_tree
// Demonstrates: EmptySumTree(), SumItem(int64_t), QuerySums(),
//               QueryItemsOrSums()
//

#include <FuzzedDataProvider.h>
#include <utils/check.h>
#include <utils/grovedb.h>
#include <utils/tempdir.h>

#include <grovedb/db.h>

#include <cstdint>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    grovedb::fuzz::TempDir tmp{"fuzz_sum_tree"};
    grovedb::Db db;
    if (!grovedb::Db::Open(tmp.path(), db).ok()) return 0;

    grovedb::OperationCost cost{};

    // Create a sum tree at root.
    grovedb::Bytes st_key = grovedb::fuzz::ConsumeKey(fdp);
    grovedb::Element sum_tree;
    CHECK_OK(grovedb::Element::EmptySumTree(sum_tree));
    CHECK_OK(db.Put(grovedb::Path{}, st_key, sum_tree, cost));

    // Insert N SumItems with fuzz-generated int64_t values.
    auto count = fdp.ConsumeIntegralInRange<size_t>(1, 16);
    for (size_t i{0}; i < count && fdp.remaining_bytes() > 0; ++i) {
        int64_t val = fdp.ConsumeIntegral<int64_t>();
        grovedb::Element si;
        if (!grovedb::Element::SumItem(val, si).ok()) continue;

        // Use index as key to guarantee uniqueness.
        grovedb::Bytes key{static_cast<uint8_t>(i)};
        (void)db.Put(grovedb::Path{st_key}, key, si, cost);
    }

    // QuerySums: read back all sum items.
    grovedb::PathQuery pq;
    auto s = grovedb::PathQuery::New(
        grovedb::Path{st_key},
        {grovedb::QueryItem::RangeFull()},
        /*limit=*/0, /*offset=*/0, pq);
    if (!s.ok()) return 0;

    std::vector<int64_t> sums;
    uint16_t skipped{0};
    (void)db.QuerySums(pq, sums, skipped, cost);

    // QueryItemsOrSums: same query, different result type.
    grovedb::PathQuery pq2;
    s = grovedb::PathQuery::New(
        grovedb::Path{st_key},
        {grovedb::QueryItem::RangeFull()},
        /*limit=*/0, /*offset=*/0, pq2);
    if (!s.ok()) return 0;

    std::vector<grovedb::QueryItemOrSum> items_or_sums;
    (void)db.QueryItemsOrSums(pq2, items_or_sums, skipped, cost);

    return 0;
}

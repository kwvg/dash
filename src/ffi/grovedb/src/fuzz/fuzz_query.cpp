// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// FUZZ TARGET: fuzz_query
// Demonstrates: PathQuery::New(), QueryItem::Key/Range/RangeFull,
//               QueryValues(), limit/offset
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
    grovedb::fuzz::TempDir tmp{"fuzz_query"};
    grovedb::Db db;
    if (!grovedb::Db::Open(tmp.path(), db).ok()) return 0;

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Pre-populate ~10 items with sequential keys.
    auto n = fdp.ConsumeIntegralInRange<size_t>(1, 10);
    for (size_t i{0}; i < n && fdp.remaining_bytes() > 0; ++i) {
        grovedb::Bytes key{static_cast<uint8_t>(i)};
        grovedb::Bytes val = grovedb::fuzz::ConsumeValue(fdp);
        grovedb::Element item;
        if (!grovedb::Element::Item(val, item).ok()) continue;
        (void)db.Put(root, key, item, cost);
    }

    // Build a QueryItem based on fuzz choice.
    grovedb::QueryItem qi;
    switch (fdp.ConsumeIntegralInRange(0, 2)) {
    case 0:
        qi = grovedb::QueryItem::RangeFull();
        break;
    case 1: {
        grovedb::Bytes k = grovedb::fuzz::ConsumeKey(fdp);
        qi = grovedb::QueryItem::Key(k);
        break;
    }
    case 2: {
        grovedb::Bytes lo = grovedb::fuzz::ConsumeKey(fdp);
        grovedb::Bytes hi = grovedb::fuzz::ConsumeKey(fdp);
        qi = grovedb::QueryItem::RangeInclusive(lo, hi);
        break;
    }
    }

    uint32_t limit = fdp.ConsumeIntegralInRange<uint32_t>(0, 20);
    uint32_t offset = fdp.ConsumeIntegralInRange<uint32_t>(0, 5);

    grovedb::PathQuery pq;
    auto s = grovedb::PathQuery::New(root, {qi}, limit, offset, pq);
    if (!s.ok()) return 0;

    std::vector<grovedb::Bytes> values;
    uint16_t skipped{0};
    s = db.QueryValues(pq, values, skipped, cost);
    if (!s.ok()) return 0;

    // When limit > 0, result size must not exceed limit.
    if (limit > 0) {
        CHECK_TRUE(values.size() <= limit);
    }

    return 0;
}

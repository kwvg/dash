// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// FUZZ TARGET: fuzz_element
// Demonstrates: Element::Item(), EmptyTree(), EmptySumTree(), SumItem()
//

#include <FuzzedDataProvider.h>
#include <utils/check.h>

#include <grovedb/element.h>
#include <grovedb/types.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider fdp(data, size);

    // Item: arbitrary value bytes.
    {
        auto len = fdp.ConsumeIntegralInRange<size_t>(0, 256);
        grovedb::Bytes value = fdp.ConsumeBytes<uint8_t>(len);
        grovedb::Element item;
        if (grovedb::Element::Item(value, item).ok()) {
            CHECK_TRUE(!item.empty());
            CHECK_TRUE(!item.data().empty());
        }
    }

    // EmptyTree: subtree placeholder.
    {
        grovedb::Element tree;
        if (grovedb::Element::EmptyTree(tree).ok()) {
            CHECK_TRUE(!tree.empty());
        }
    }

    // EmptySumTree: sum-tree placeholder.
    {
        grovedb::Element sum_tree;
        if (grovedb::Element::EmptySumTree(sum_tree).ok()) {
            CHECK_TRUE(!sum_tree.empty());
        }
    }

    // SumItem: i64 value.
    {
        int64_t val = fdp.ConsumeIntegral<int64_t>();
        grovedb::Element sum_item;
        if (grovedb::Element::SumItem(val, sum_item).ok()) {
            CHECK_TRUE(!sum_item.empty());
        }
    }

    return 0;
}

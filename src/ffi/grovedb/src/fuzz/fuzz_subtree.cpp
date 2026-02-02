// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// FUZZ TARGET: fuzz_subtree
// Demonstrates: EmptyTree(), Put() at nested paths, SubtreeExists(),
//               IsEmptyTree(), FindSubtrees()
//

#include <FuzzedDataProvider.h>
#include <utils/check.h>
#include <utils/grovedb.h>
#include <utils/tempdir.h>

#include <grovedb/db.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    grovedb::fuzz::TempDir tmp{"fuzz_subtree"};
    grovedb::Db db;
    if (!grovedb::Db::Open(tmp.path(), db).ok()) return 0;

    grovedb::OperationCost cost{};

    // Build a subtree chain: root -> s1 -> s2.
    grovedb::Bytes s1_key = grovedb::fuzz::ConsumeKey(fdp);
    grovedb::Bytes s2_key = grovedb::fuzz::ConsumeKey(fdp);

    grovedb::Element tree;
    CHECK_OK(grovedb::Element::EmptyTree(tree));

    // Insert s1 at root.
    CHECK_OK(db.Put(grovedb::Path{}, s1_key, tree, cost));

    // s1 should exist and be empty.
    bool exists{false};
    CHECK_OK(db.SubtreeExists(grovedb::Path{s1_key}, exists, cost));
    CHECK_TRUE(exists);

    bool empty{false};
    CHECK_OK(db.IsEmptyTree(grovedb::Path{s1_key}, empty, cost));
    CHECK_TRUE(empty);

    // Insert s2 under s1.
    CHECK_OK(grovedb::Element::EmptyTree(tree));
    CHECK_OK(db.Put(grovedb::Path{s1_key}, s2_key, tree, cost));

    // s2 should exist.
    CHECK_OK(db.SubtreeExists(grovedb::Path{s1_key, s2_key}, exists, cost));
    CHECK_TRUE(exists);

    // s1 should no longer be empty (it contains s2).
    CHECK_OK(db.IsEmptyTree(grovedb::Path{s1_key}, empty, cost));
    CHECK_TRUE(!empty);

    // Insert an item inside s2.
    grovedb::Bytes item_key = grovedb::fuzz::ConsumeKey(fdp);
    grovedb::Bytes item_val = grovedb::fuzz::ConsumeValue(fdp);
    grovedb::Element item;
    if (grovedb::Element::Item(item_val, item).ok()) {
        CHECK_OK(db.Put(grovedb::Path{s1_key, s2_key}, item_key, item, cost));
    }

    // FindSubtrees from root.
    std::vector<grovedb::Path> subtrees;
    CHECK_OK(db.FindSubtrees(grovedb::Path{}, subtrees, cost));

    return 0;
}

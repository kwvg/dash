// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// FUZZ TARGET: fuzz_put_get
// Demonstrates: Db::Put(), GetDirect(), KeyExists(), GetOptional()
//

#include <FuzzedDataProvider.h>
#include <utils/check.h>
#include <utils/grovedb.h>
#include <utils/tempdir.h>

#include <grovedb/db.h>

#include <optional>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    grovedb::fuzz::TempDir tmp{"fuzz_put_get"};
    grovedb::Db db;
    if (!grovedb::Db::Open(tmp.path(), db).ok()) return 0;

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Create an Item with fuzz-generated value.
    grovedb::Bytes key = grovedb::fuzz::ConsumeKey(fdp);
    grovedb::Bytes value = grovedb::fuzz::ConsumeValue(fdp);
    grovedb::Element item;
    if (!grovedb::Element::Item(value, item).ok()) return 0;

    // Put at root level.
    CHECK_OK(db.Put(root, key, item, cost));

    // GetDirect: read back without following references.
    grovedb::Element fetched;
    CHECK_OK(db.GetDirect(root, key, fetched, cost));
    CHECK_TRUE(!fetched.empty());
    CHECK_EQ(fetched.data(), item.data());

    // KeyExists: should be true for key we just wrote.
    bool exists{false};
    CHECK_OK(db.KeyExists(root, key, exists, cost));
    CHECK_TRUE(exists);

    // GetOptional: non-existent key yields nullopt.
    grovedb::Bytes missing = grovedb::fuzz::ConsumeKey(fdp);
    std::optional<grovedb::Element> maybe;
    CHECK_OK(db.GetOptional(root, missing, maybe, cost));
    // (may or may not exist if the fuzzer reuses the same key bytes)

    return 0;
}

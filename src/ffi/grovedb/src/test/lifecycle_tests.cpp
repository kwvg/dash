// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <grovedb/db.h>

#include <test/util/tempdir.h>

BOOST_AUTO_TEST_SUITE(lifecycle_tests)

BOOST_AUTO_TEST_CASE(test_open_and_destroy)
{
    grovedb::test::TempDir tmp{"grovedb_test_open"};
    grovedb::Db db;
    auto status = grovedb::Db::Open(tmp.PathToString(), db);
    BOOST_CHECK(status.ok());
}

BOOST_AUTO_TEST_CASE(test_root_hash)
{
    grovedb::test::TempDir tmp{"grovedb_test_root_hash"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::Hash hash{};
    grovedb::OperationCost cost{};
    auto status = db.GetRootHash(hash, cost);
    BOOST_CHECK(status.ok());
    BOOST_CHECK_EQUAL(hash.size(), 32);
}

BOOST_AUTO_TEST_CASE(test_flush)
{
    grovedb::test::TempDir tmp{"grovedb_test_flush"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    BOOST_CHECK(db.Flush().ok());
}

BOOST_AUTO_TEST_CASE(test_destroy)
{
    grovedb::test::TempDir tmp{"grovedb_test_destroy"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    BOOST_CHECK(db.Destroy().ok());

    grovedb::Hash hash{};
    grovedb::OperationCost cost{};
    auto status = db.GetRootHash(hash, cost);
    BOOST_CHECK(status.ok());
    BOOST_CHECK_EQUAL(hash.size(), 32);
}

BOOST_AUTO_TEST_CASE(test_verify_integrity)
{
    grovedb::test::TempDir tmp{"grovedb_test_verify"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    bool ok{false};
    auto status = db.VerifyIntegrity(ok);
    BOOST_CHECK(status.ok());
    BOOST_CHECK(ok);
}

BOOST_AUTO_TEST_SUITE_END()

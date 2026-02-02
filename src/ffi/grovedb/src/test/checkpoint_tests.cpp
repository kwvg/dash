// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <grovedb/db.h>

#include <test/util/tempdir.h>

BOOST_AUTO_TEST_SUITE(checkpoint_tests)

BOOST_AUTO_TEST_CASE(test_create_open_checkpoint)
{
    grovedb::test::TempDir tmp_db{"grovedb_test_checkpoint_db"};
    grovedb::test::TempDir tmp_cp{"grovedb_test_checkpoint_cp"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp_db.PathToString(), db).ok());

    grovedb::OperationCost cost{};

    // Insert data into the database.
    grovedb::Element item;
    BOOST_REQUIRE(grovedb::Element::Item(grovedb::Bytes{'v', 'a', 'l'}, item).ok());
    BOOST_REQUIRE(db.Put(grovedb::Path{}, grovedb::Bytes{'k'}, item, cost).ok());

    // Create a checkpoint.
    auto cp_path = tmp_cp.PathToString() + "/snap1";
    BOOST_REQUIRE(db.CreateCheckpoint(cp_path).ok());

    // Open the checkpoint as a separate Db instance.
    grovedb::Db cp_db;
    BOOST_REQUIRE(grovedb::Db::OpenCheckpoint(cp_path, cp_db).ok());

    // Verify the checkpoint has the data.
    grovedb::Element fetched;
    BOOST_REQUIRE(cp_db.Get(grovedb::Path{}, grovedb::Bytes{'k'}, fetched, cost).ok());
    BOOST_CHECK(!fetched.empty());
}

BOOST_AUTO_TEST_CASE(test_checkpoint_is_snapshot)
{
    grovedb::test::TempDir tmp_db{"grovedb_test_checkpoint_snap"};
    grovedb::test::TempDir tmp_cp{"grovedb_test_checkpoint_snap_cp"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp_db.PathToString(), db).ok());

    grovedb::OperationCost cost{};

    // Insert initial data.
    grovedb::Element item1;
    BOOST_REQUIRE(grovedb::Element::Item(grovedb::Bytes{'v', '1'}, item1).ok());
    BOOST_REQUIRE(db.Put(grovedb::Path{}, grovedb::Bytes{'k', '1'}, item1, cost).ok());

    // Create a checkpoint.
    auto cp_path = tmp_cp.PathToString() + "/snap";
    BOOST_REQUIRE(db.CreateCheckpoint(cp_path).ok());

    // Insert more data AFTER the checkpoint.
    grovedb::Element item2;
    BOOST_REQUIRE(grovedb::Element::Item(grovedb::Bytes{'v', '2'}, item2).ok());
    BOOST_REQUIRE(db.Put(grovedb::Path{}, grovedb::Bytes{'k', '2'}, item2, cost).ok());

    // Open the checkpoint — it should NOT have the second key.
    grovedb::Db cp_db;
    BOOST_REQUIRE(grovedb::Db::OpenCheckpoint(cp_path, cp_db).ok());

    // k1 exists in the checkpoint.
    grovedb::Element fetched;
    BOOST_REQUIRE(cp_db.Get(grovedb::Path{}, grovedb::Bytes{'k', '1'}, fetched, cost).ok());
    BOOST_CHECK(!fetched.empty());

    // k2 does NOT exist in the checkpoint.
    grovedb::Element fetched2;
    auto status = cp_db.Get(grovedb::Path{}, grovedb::Bytes{'k', '2'}, fetched2, cost);
    BOOST_CHECK(!status.ok());
}

BOOST_AUTO_TEST_CASE(test_delete_checkpoint)
{
    grovedb::test::TempDir tmp_db{"grovedb_test_delete_cp_db"};
    grovedb::test::TempDir tmp_cp{"grovedb_test_delete_cp_cp"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp_db.PathToString(), db).ok());

    // Create a checkpoint.
    auto cp_path = tmp_cp.PathToString() + "/snap_del";
    BOOST_REQUIRE(db.CreateCheckpoint(cp_path).ok());

    // Delete it.
    BOOST_REQUIRE(grovedb::Db::DeleteCheckpoint(cp_path).ok());

    // Trying to open the deleted checkpoint should fail.
    grovedb::Db cp_db;
    auto status = grovedb::Db::OpenCheckpoint(cp_path, cp_db);
    BOOST_CHECK(!status.ok());
}

BOOST_AUTO_TEST_SUITE_END()

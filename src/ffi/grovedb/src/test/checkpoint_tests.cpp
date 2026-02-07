// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <test/util/tempdir.h>

#include <grovedb/db.h>
#include <grovedb/element.h>

#include <boost/test/unit_test.hpp>

#include <string>

BOOST_AUTO_TEST_SUITE(checkpoint_tests)

namespace {
// Helper: build a checkpoint path as a subdirectory of the test dir.
// CreateCheckpoint expects the target directory to NOT exist yet, so we
// cannot use TempDir (which pre-creates the directory).
std::string CkptPath(const grovedb::test::TempDir& dir, const char* name)
{
  return dir.PathToString() + "/" + name;
}
} // anonymous namespace

BOOST_AUTO_TEST_CASE(create_and_open_checkpoint)
{
  grovedb::test::TempDir dir("ckpt_create_open");
  auto db = grovedb::Db::Open(dir.PathToString() + "/db");
  BOOST_REQUIRE(db.has_value());

  // Insert an item so the checkpoint has data.
  grovedb::Path path{};
  auto key = grovedb::Bytes::FromString("ck");
  auto elem = grovedb::Element::Item(grovedb::Bytes::FromString("cv"));
  BOOST_REQUIRE(elem.has_value());
  BOOST_REQUIRE(db->Put(path, key, *elem).has_value());

  // Create checkpoint in a non-existent subdirectory.
  std::string ckpt = CkptPath(dir, "ckpt");
  auto create = db->CreateCheckpoint(ckpt);
  BOOST_REQUIRE(create.has_value());

  // Open the checkpoint as a separate database handle.
  auto ckpt_db = grovedb::Db::OpenCheckpoint(ckpt);
  BOOST_REQUIRE(ckpt_db.has_value());

  // Data written before the checkpoint should be readable.
  auto got = ckpt_db->Get(path, key);
  BOOST_REQUIRE(got.has_value());
  BOOST_CHECK(got->value() == *elem);
}

BOOST_AUTO_TEST_CASE(checkpoint_is_snapshot)
{
  grovedb::test::TempDir dir("ckpt_snapshot");
  auto db = grovedb::Db::Open(dir.PathToString() + "/db");
  BOOST_REQUIRE(db.has_value());

  // Insert first item.
  grovedb::Path path{};
  auto key1 = grovedb::Bytes::FromString("k1");
  auto elem1 = grovedb::Element::Item(grovedb::Bytes::FromString("v1"));
  BOOST_REQUIRE(elem1.has_value());
  BOOST_REQUIRE(db->Put(path, key1, *elem1).has_value());

  // Create checkpoint.
  std::string ckpt = CkptPath(dir, "ckpt");
  BOOST_REQUIRE(db->CreateCheckpoint(ckpt).has_value());

  // Insert second item AFTER checkpoint.
  auto key2 = grovedb::Bytes::FromString("k2");
  auto elem2 = grovedb::Element::Item(grovedb::Bytes::FromString("v2"));
  BOOST_REQUIRE(elem2.has_value());
  BOOST_REQUIRE(db->Put(path, key2, *elem2).has_value());

  // Open checkpoint — key2 should NOT be visible.
  auto ckpt_db = grovedb::Db::OpenCheckpoint(ckpt);
  BOOST_REQUIRE(ckpt_db.has_value());

  auto k1 = ckpt_db->KeyExists(path, key1);
  BOOST_REQUIRE(k1.has_value());
  BOOST_CHECK(k1->value());

  auto k2 = ckpt_db->KeyExists(path, key2);
  BOOST_REQUIRE(k2.has_value());
  BOOST_CHECK(!k2->value());
}

BOOST_AUTO_TEST_CASE(delete_checkpoint)
{
  grovedb::test::TempDir dir("ckpt_delete");
  auto db = grovedb::Db::Open(dir.PathToString() + "/db");
  BOOST_REQUIRE(db.has_value());

  std::string ckpt = CkptPath(dir, "ckpt");
  BOOST_REQUIRE(db->CreateCheckpoint(ckpt).has_value());

  // Delete the checkpoint.
  auto del = grovedb::Db::DeleteCheckpoint(ckpt);
  BOOST_CHECK(del.has_value());
}

BOOST_AUTO_TEST_CASE(open_nonexistent_checkpoint_fails)
{
  auto result = grovedb::Db::OpenCheckpoint("/tmp/grovedb_nonexistent_ckpt_12345");
  BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(checkpoint_root_hash_matches)
{
  grovedb::test::TempDir dir("ckpt_hash");
  auto db = grovedb::Db::Open(dir.PathToString() + "/db");
  BOOST_REQUIRE(db.has_value());

  // Insert data and get root hash.
  grovedb::Path path{};
  auto key = grovedb::Bytes::FromString("hk");
  auto elem = grovedb::Element::Item(grovedb::Bytes::FromString("hv"));
  BOOST_REQUIRE(elem.has_value());
  BOOST_REQUIRE(db->Put(path, key, *elem).has_value());

  auto hash_before = db->GetRootHash();
  BOOST_REQUIRE(hash_before.has_value());

  // Create and open checkpoint.
  std::string ckpt = CkptPath(dir, "ckpt");
  BOOST_REQUIRE(db->CreateCheckpoint(ckpt).has_value());

  auto ckpt_db = grovedb::Db::OpenCheckpoint(ckpt);
  BOOST_REQUIRE(ckpt_db.has_value());

  auto ckpt_hash = ckpt_db->GetRootHash();
  BOOST_REQUIRE(ckpt_hash.has_value());

  // Root hashes should match.
  BOOST_CHECK(hash_before->value() == ckpt_hash->value());
}

BOOST_AUTO_TEST_SUITE_END()

// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <test/util/tempdir.h>

#include <grovedb/db.h>
#include <grovedb/element.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(delete_tests)

BOOST_AUTO_TEST_CASE(delete_existing_item)
{
  grovedb::test::TempDir dir("del_existing");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path path{};
  grovedb::Bytes key{'k'};
  auto elem = grovedb::Element::Item(grovedb::Bytes{'v'});
  BOOST_REQUIRE(elem.has_value());
  BOOST_REQUIRE(db->Put(path, key, *elem).has_value());

  auto del = db->Delete(path, key);
  BOOST_REQUIRE(del.has_value());

  // Key should no longer exist.
  auto exists = db->KeyExists(path, key);
  BOOST_REQUIRE(exists.has_value());
  BOOST_CHECK(!exists->value());
}

BOOST_AUTO_TEST_CASE(delete_missing_key)
{
  grovedb::test::TempDir dir("del_missing");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path path{};
  grovedb::Bytes key{'x'};
  auto del = db->Delete(path, key);
  // Deleting a missing key may succeed or error — just verify no crash.
  (void)del;
}

BOOST_AUTO_TEST_CASE(delete_if_empty_on_empty_subtree)
{
  grovedb::test::TempDir dir("del_if_empty_ok");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path path{};
  grovedb::Bytes key{'s'};

  // Create an empty subtree.
  auto tree = grovedb::Element::EmptyTree();
  BOOST_REQUIRE(tree.has_value());
  BOOST_REQUIRE(db->Put(path, key, *tree).has_value());

  // DeleteIfEmpty should succeed and return true.
  auto del = db->DeleteIfEmpty(path, key);
  BOOST_REQUIRE(del.has_value());
  BOOST_CHECK(del->value());

  // Key should no longer exist.
  auto exists = db->KeyExists(path, key);
  BOOST_REQUIRE(exists.has_value());
  BOOST_CHECK(!exists->value());
}

BOOST_AUTO_TEST_CASE(delete_if_empty_on_non_empty_subtree)
{
  grovedb::test::TempDir dir("del_if_empty_skip");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path root{};
  grovedb::Bytes subtree_key{'s'};
  grovedb::Bytes item_key{'i'};

  // Create a subtree and put an item in it.
  auto tree = grovedb::Element::EmptyTree();
  BOOST_REQUIRE(tree.has_value());
  BOOST_REQUIRE(db->Put(root, subtree_key, *tree).has_value());

  grovedb::Path subtree{subtree_key};
  auto elem = grovedb::Element::Item(grovedb::Bytes{'v'});
  BOOST_REQUIRE(elem.has_value());
  BOOST_REQUIRE(db->Put(subtree, item_key, *elem).has_value());

  // DeleteIfEmpty should not delete the non-empty subtree.
  auto del = db->DeleteIfEmpty(root, subtree_key);
  BOOST_REQUIRE(del.has_value());
  BOOST_CHECK(!del->value());

  // Subtree should still exist.
  auto exists = db->KeyExists(root, subtree_key);
  BOOST_REQUIRE(exists.has_value());
  BOOST_CHECK(exists->value());
}

BOOST_AUTO_TEST_CASE(prune_empty_ancestors_cascade)
{
  grovedb::test::TempDir dir("del_prune");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path root{};
  grovedb::Bytes a{'a'};
  grovedb::Bytes b{'b'};
  grovedb::Bytes item_key{'i'};

  // Create nested subtrees: root -> a -> b
  auto tree = grovedb::Element::EmptyTree();
  BOOST_REQUIRE(tree.has_value());
  BOOST_REQUIRE(db->Put(root, a, *tree).has_value());

  grovedb::Path path_a{a};
  BOOST_REQUIRE(db->Put(path_a, b, *tree).has_value());

  // Insert an item at root/a/b/i
  grovedb::Path path_ab{a, b};
  auto elem = grovedb::Element::Item(grovedb::Bytes{'v'});
  BOOST_REQUIRE(elem.has_value());
  BOOST_REQUIRE(db->Put(path_ab, item_key, *elem).has_value());

  // Delete the item and prune empty ancestors.
  auto result = db->PruneEmptyAncestors(path_ab, item_key);
  BOOST_REQUIRE(result.has_value());
  // Should have removed at least 1 level (the item itself).
  BOOST_CHECK(result->value() >= 1u);
}

BOOST_AUTO_TEST_CASE(clear_subtree)
{
  grovedb::test::TempDir dir("del_clear");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path root{};
  grovedb::Bytes subtree_key{'s'};
  grovedb::Bytes k1{'a'};
  grovedb::Bytes k2{'b'};

  // Create a subtree and insert items.
  auto tree = grovedb::Element::EmptyTree();
  BOOST_REQUIRE(tree.has_value());
  BOOST_REQUIRE(db->Put(root, subtree_key, *tree).has_value());

  grovedb::Path subtree{subtree_key};
  auto elem = grovedb::Element::Item(grovedb::Bytes{'v'});
  BOOST_REQUIRE(elem.has_value());
  BOOST_REQUIRE(db->Put(subtree, k1, *elem).has_value());
  BOOST_REQUIRE(db->Put(subtree, k2, *elem).has_value());

  // Subtree should not be empty.
  auto before = db->IsEmptyTree(subtree);
  BOOST_REQUIRE(before.has_value());
  BOOST_CHECK(!before->value());

  // Clear the subtree.
  auto clear_result = db->Clear(subtree);
  BOOST_REQUIRE(clear_result.has_value());

  // Subtree should now be empty.
  auto after = db->IsEmptyTree(subtree);
  BOOST_REQUIRE(after.has_value());
  BOOST_CHECK(after->value());
}

BOOST_AUTO_TEST_CASE(delete_within_transaction)
{
  grovedb::test::TempDir dir("del_tx");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  grovedb::Path path{};
  grovedb::Bytes key{'t'};
  auto elem = grovedb::Element::Item(grovedb::Bytes{'v'});
  BOOST_REQUIRE(elem.has_value());
  BOOST_REQUIRE(db->Put(path, key, *elem).has_value());

  // Delete within a transaction and commit.
  auto tx = db->BeginTransaction();
  BOOST_REQUIRE(tx.has_value());
  BOOST_REQUIRE(db->Delete(path, key, *tx).has_value());
  BOOST_REQUIRE(db->Commit(*tx).has_value());

  // Key should be gone after commit.
  auto exists = db->KeyExists(path, key);
  BOOST_REQUIRE(exists.has_value());
  BOOST_CHECK(!exists->value());
}

BOOST_AUTO_TEST_CASE(deep_nested_prune)
{
  grovedb::test::TempDir dir("del_deep_prune");
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());

  // Create a chain of 10 nested empty subtrees.
  static constexpr int DEPTH = 10;
  grovedb::Path path{};
  for (int i = 0; i < DEPTH; ++i) {
    grovedb::Bytes key{'l', static_cast<uint8_t>(i)};
    auto tree = grovedb::Element::EmptyTree();
    BOOST_REQUIRE(tree.has_value());
    BOOST_REQUIRE(db->Put(path, key, *tree).has_value());
    path.push_back(key);
  }

  // Insert a single Item at the bottom.
  grovedb::Bytes leaf_key{'i'};
  auto elem = grovedb::Element::Item(grovedb::Bytes{'v'});
  BOOST_REQUIRE(elem.has_value());
  BOOST_REQUIRE(db->Put(path, leaf_key, *elem).has_value());

  // Prune the item and its empty ancestors.
  auto result = db->PruneEmptyAncestors(path, leaf_key);
  BOOST_REQUIRE(result.has_value());
  // Should have removed at least DEPTH levels.
  BOOST_CHECK(result->value() >= static_cast<uint32_t>(DEPTH));
}

BOOST_AUTO_TEST_SUITE_END()

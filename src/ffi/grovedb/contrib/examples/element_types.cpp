// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <test/util/tempdir.h>

#include <grovedb/db.h>
#include <grovedb/element.h>

#include <util/logging.h>

#include <cstdlib>
#include <string>

int main()
{
  Log(INFO, "=== Element Types ===");

  // -------------------------------------------------------------------------
  // 1. Create all four element types via factories and log each one.
  // -------------------------------------------------------------------------

  auto item = grovedb::Element::Item(grovedb::Bytes::FromString("hello world"));
  if (!item.has_value()) {
    Log(ERROR, "failed to create Item: {}", item.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "created Item element ({} raw bytes)", item->data().size());

  auto tree = grovedb::Element::EmptyTree();
  if (!tree.has_value()) {
    Log(ERROR, "failed to create EmptyTree: {}", tree.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "created EmptyTree element ({} raw bytes)", tree->data().size());

  auto sum_tree = grovedb::Element::EmptySumTree();
  if (!sum_tree.has_value()) {
    Log(ERROR, "failed to create EmptySumTree: {}", sum_tree.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "created EmptySumTree element ({} raw bytes)", sum_tree->data().size());

  auto sum_item = grovedb::Element::SumItem(42);
  if (!sum_item.has_value()) {
    Log(ERROR, "failed to create SumItem: {}", sum_item.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "created SumItem(42) element ({} raw bytes)", sum_item->data().size());

  // -------------------------------------------------------------------------
  // 2. Store each element at root under different keys.
  // -------------------------------------------------------------------------

  grovedb::test::TempDir dir("ex_element_types");
  Log(INFO, "opening database at {}", dir.PathToString());

  auto result = grovedb::Db::Open(dir.PathToString());
  if (!result.has_value()) {
    Log(ERROR, "failed to open database: {}", result.error().message());
    return EXIT_FAILURE;
  }
  auto db = std::move(result).value();
  Log(INFO, "database opened successfully");

  grovedb::Path root{};

  auto cost1 = db.Put(root, grovedb::Bytes::FromString("item"), *item);
  if (!cost1.has_value()) {
    Log(ERROR, "put Item failed: {}", cost1.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO,
      "stored 'item': {} seeks, {} bytes added",
      cost1->m_seek_count,
      cost1->m_storage_added_bytes);

  auto cost2 = db.Put(root, grovedb::Bytes::FromString("tree"), *tree);
  if (!cost2.has_value()) {
    Log(ERROR, "put EmptyTree failed: {}", cost2.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO,
      "stored 'tree': {} seeks, {} bytes added",
      cost2->m_seek_count,
      cost2->m_storage_added_bytes);

  auto cost3 = db.Put(root, grovedb::Bytes::FromString("sum_tree"), *sum_tree);
  if (!cost3.has_value()) {
    Log(ERROR, "put EmptySumTree failed: {}", cost3.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO,
      "stored 'sum_tree': {} seeks, {} bytes added",
      cost3->m_seek_count,
      cost3->m_storage_added_bytes);

  // SumItem must be stored under a SumTree, not root.
  grovedb::Path sum_path{grovedb::Bytes::FromString("sum_tree")};
  auto cost4 = db.Put(sum_path, grovedb::Bytes::FromString("sum_item"), *sum_item);
  if (!cost4.has_value()) {
    Log(ERROR, "put SumItem failed: {}", cost4.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO,
      "stored 'sum_item' under 'sum_tree': {} seeks, {} bytes added",
      cost4->m_seek_count,
      cost4->m_storage_added_bytes);

  // -------------------------------------------------------------------------
  // 3. Retrieve each element and log raw data sizes.
  // -------------------------------------------------------------------------

  auto got_item = db.Get(root, grovedb::Bytes::FromString("item"));
  if (!got_item.has_value()) {
    Log(ERROR, "get 'item' failed: {}", got_item.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "retrieved 'item': {} raw bytes", got_item->value().data().size());

  auto got_tree = db.Get(root, grovedb::Bytes::FromString("tree"));
  if (!got_tree.has_value()) {
    Log(ERROR, "get 'tree' failed: {}", got_tree.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "retrieved 'tree': {} raw bytes", got_tree->value().data().size());

  auto got_sum_tree = db.Get(root, grovedb::Bytes::FromString("sum_tree"));
  if (!got_sum_tree.has_value()) {
    Log(ERROR, "get 'sum_tree' failed: {}", got_sum_tree.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "retrieved 'sum_tree': {} raw bytes", got_sum_tree->value().data().size());

  auto got_sum_item = db.Get(sum_path, grovedb::Bytes::FromString("sum_item"));
  if (!got_sum_item.has_value()) {
    Log(ERROR, "get 'sum_item' failed: {}", got_sum_item.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "retrieved 'sum_item': {} raw bytes", got_sum_item->value().data().size());

  // -------------------------------------------------------------------------
  // 4. Show empty() on default vs constructed elements.
  // -------------------------------------------------------------------------

  grovedb::Element default_elem;
  Log(INFO, "default-constructed element is empty: {}", default_elem.empty());
  Log(INFO, "factory-constructed Item is empty: {}", item->empty());

  Log(INFO, "done");
  return EXIT_SUCCESS;
}

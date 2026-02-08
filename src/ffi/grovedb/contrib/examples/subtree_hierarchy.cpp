// Copyright (c) 2026-present, The Dash Core developers
// Copyright (c) 2011-present, Facebook, Inc.
// Distributed under the Apache 2.0 license, see the accompanying
// file LICENSE.APACHE2 or https://opensource.org/license/apache-2-0
//
// Adapted from RocksDb's example/column_families_example.cc

#include <test/util/tempdir.h>

#include <grovedb/db.h>
#include <grovedb/element.h>

#include <util/logging.h>

#include <cstdlib>
#include <string>

int main()
{
  Log(INFO, "=== Subtree Hierarchy ===");

  grovedb::test::TempDir dir("ex_subtree_hierarchy");
  auto result = grovedb::Db::Open(dir.PathToString());
  if (!result.has_value()) {
    Log(ERROR, "failed to open database: {}", result.error().message());
    return EXIT_FAILURE;
  }
  auto db = std::move(result).value();
  Log(INFO, "database opened at {}", dir.PathToString());

  grovedb::Path root{};

  // -------------------------------------------------------------------------
  // 1. Create tree: root → "users" → {"alice", "bob"}, root → "config".
  // -------------------------------------------------------------------------

  Log(INFO, "--- creating subtree hierarchy ---");

  auto cu = grovedb::Element::EmptyTree().and_then([&](grovedb::Element e) {
    return db.Put(root, grovedb::Bytes::FromString("users"), e);
  });
  if (!cu.has_value()) {
    Log(ERROR, "put 'users' failed: {}", cu.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "created subtree 'users'");

  grovedb::Path users_path{grovedb::Bytes::FromString("users")};

  for (const char* name : {"alice", "bob"}) {
    auto ct = grovedb::Element::EmptyTree().and_then([&](grovedb::Element e) {
      return db.Put(users_path, grovedb::Bytes::FromString(name), e);
    });
    if (!ct.has_value()) {
      Log(ERROR, "put '{}' subtree failed: {}", name, ct.error().message());
      return EXIT_FAILURE;
    }
    Log(INFO, "created subtree 'users/{}'", name);
  }

  auto cc = grovedb::Element::EmptyTree().and_then([&](grovedb::Element e) {
    return db.Put(root, grovedb::Bytes::FromString("config"), e);
  });
  if (!cc.has_value()) {
    Log(ERROR, "put 'config' failed: {}", cc.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "created subtree 'config'");

  // -------------------------------------------------------------------------
  // 2. Insert data in leaves.
  // -------------------------------------------------------------------------

  Log(INFO, "--- inserting data in leaves ---");
  for (const char* name : {"alice", "bob"}) {
    grovedb::Path user_path{grovedb::Bytes::FromString("users"), grovedb::Bytes::FromString(name)};
    auto email_val = grovedb::Bytes::FromString(std::string(name) + "@example.com");
    auto c = grovedb::Element::Item(email_val).and_then([&](grovedb::Element e) {
      return db.Put(user_path, grovedb::Bytes::FromString("email"), e);
    });
    if (!c.has_value()) {
      Log(ERROR, "put email for {} failed: {}", name, c.error().message());
      return EXIT_FAILURE;
    }
    Log(INFO, "inserted users/{}/email = '{}@example.com'", name, name);
  }

  // -------------------------------------------------------------------------
  // 3. SubtreeExists for present/absent paths.
  // -------------------------------------------------------------------------

  Log(INFO, "--- SubtreeExists ---");

  grovedb::Path alice_path{
      grovedb::Bytes::FromString("users"), grovedb::Bytes::FromString("alice")
  };
  auto se1 = db.SubtreeExists(alice_path);
  if (!se1.has_value()) {
    Log(ERROR, "SubtreeExists failed: {}", se1.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "SubtreeExists(users/alice) = {}", se1->value());

  grovedb::Path missing_path{
      grovedb::Bytes::FromString("users"), grovedb::Bytes::FromString("charlie")
  };
  auto se2 = db.SubtreeExists(missing_path);
  if (!se2.has_value()) {
    Log(ERROR, "SubtreeExists failed: {}", se2.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "SubtreeExists(users/charlie) = {} (does not exist)", se2->value());

  // -------------------------------------------------------------------------
  // 4. IsEmptyTree for empty/non-empty subtrees.
  // -------------------------------------------------------------------------

  Log(INFO, "--- IsEmptyTree ---");

  auto ie1 = db.IsEmptyTree(alice_path);
  if (!ie1.has_value()) {
    Log(ERROR, "IsEmptyTree failed: {}", ie1.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "IsEmptyTree(users/alice) = {} (has email)", ie1->value());

  grovedb::Path config_path{grovedb::Bytes::FromString("config")};
  auto ie2 = db.IsEmptyTree(config_path);
  if (!ie2.has_value()) {
    Log(ERROR, "IsEmptyTree failed: {}", ie2.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "IsEmptyTree(config) = {} (no items added)", ie2->value());

  // -------------------------------------------------------------------------
  // 5. Get nested data.
  // -------------------------------------------------------------------------

  Log(INFO, "--- Get nested data ---");
  auto get1 = db.Get(alice_path, grovedb::Bytes::FromString("email"));
  if (!get1.has_value()) {
    Log(ERROR, "Get failed: {}", get1.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "Get(users/alice/email): {} raw bytes", get1->value().data().size());

  // -------------------------------------------------------------------------
  // 6. FindSubtrees from root.
  // -------------------------------------------------------------------------

  Log(INFO, "--- FindSubtrees ---");
  auto found = db.FindSubtrees(root);
  if (!found.has_value()) {
    Log(ERROR, "FindSubtrees failed: {}", found.error().message());
    return EXIT_FAILURE;
  }

  Log(INFO, "FindSubtrees(root) found {} paths:", found->value().size());
  for (const auto& path : found->value()) {
    std::string path_str;
    if (path.empty()) {
      path_str = "(root)";
    } else {
      for (const auto& seg : path) {
        if (!path_str.empty()) {
          path_str += "/";
        }
        path_str += seg.ToString();
      }
    }
    Log(INFO, "  {}", path_str);
  }

  Log(INFO, "done");
  return EXIT_SUCCESS;
}

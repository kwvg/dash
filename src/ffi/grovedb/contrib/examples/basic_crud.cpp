// Copyright (c) 2026-present, The Dash Core developers
// Copyright (c) 2011-present, Facebook, Inc.
// Distributed under the Apache 2.0 license, see the accompanying
// file LICENSE.APACHE2 or https://opensource.org/license/apache-2-0
//
// Adapted from RocksDb's example/simple_example.cc

#include <test/util/tempdir.h>

#include <grovedb/db.h>
#include <grovedb/element.h>

#include <util/logging.h>

#include <cstdlib>
#include <string>

int main()
{
  Log(INFO, "=== Basic CRUD ===");

  grovedb::test::TempDir dir("ex_basic_crud");
  Log(INFO, "opening database at {}", dir.PathToString());

  auto result = grovedb::Db::Open(dir.PathToString());
  if (!result.has_value()) {
    Log(ERROR, "failed to open database: {}", result.error().message());
    return EXIT_FAILURE;
  }
  auto db = std::move(result).value();
  Log(INFO, "database opened successfully");

  grovedb::Path root{};

  // -------------------------------------------------------------------------
  // 1. Create — Put an item.
  // -------------------------------------------------------------------------

  auto put1 = grovedb::Element::Item(grovedb::Bytes::FromString("Hello GroveDB!"))
                  .and_then([&](grovedb::Element e) {
                    return db.Put(root, grovedb::Bytes::FromString("greeting"), e);
                  });
  if (!put1.has_value()) {
    Log(ERROR, "put failed: {}", put1.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO,
      "Put('greeting', 'Hello GroveDB!'): {} seeks, {} bytes added",
      put1->m_seek_count,
      put1->m_storage_added_bytes);

  // -------------------------------------------------------------------------
  // 2. Read — Get the item back.
  // -------------------------------------------------------------------------

  auto get1 = db.Get(root, grovedb::Bytes::FromString("greeting"));
  if (!get1.has_value()) {
    Log(ERROR, "get failed: {}", get1.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO,
      "Get('greeting'): {} raw bytes, {} seeks",
      get1->value().data().size(),
      get1->cost().m_seek_count);

  // -------------------------------------------------------------------------
  // 3. KeyExists — check presence.
  // -------------------------------------------------------------------------

  auto exists1 = db.KeyExists(root, grovedb::Bytes::FromString("greeting"));
  if (!exists1.has_value()) {
    Log(ERROR, "key exists check failed: {}", exists1.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "KeyExists('greeting') = {}", exists1->value());

  auto exists2 = db.KeyExists(root, grovedb::Bytes::FromString("missing"));
  if (!exists2.has_value()) {
    Log(ERROR, "key exists check failed: {}", exists2.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "KeyExists('missing') = {}", exists2->value());

  // -------------------------------------------------------------------------
  // 4. Insert another item.
  // -------------------------------------------------------------------------

  auto put2 =
      grovedb::Element::Item(grovedb::Bytes::FromString("Dash")).and_then([&](grovedb::Element e) {
        return db.Put(root, grovedb::Bytes::FromString("name"), e);
      });
  if (!put2.has_value()) {
    Log(ERROR, "put failed: {}", put2.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO,
      "Put('name', 'Dash'): {} seeks, {} bytes added",
      put2->m_seek_count,
      put2->m_storage_added_bytes);

  // -------------------------------------------------------------------------
  // 5. Delete — remove the greeting.
  // -------------------------------------------------------------------------

  auto del = db.Delete(root, grovedb::Bytes::FromString("greeting"));
  if (!del.has_value()) {
    Log(ERROR, "delete failed: {}", del.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO,
      "Delete('greeting'): {} seeks, {} bytes removed",
      del->m_seek_count,
      del->m_storage_removed_bytes);

  // Confirm deletion.
  auto exists3 = db.KeyExists(root, grovedb::Bytes::FromString("greeting"));
  if (!exists3.has_value()) {
    Log(ERROR, "key exists check failed: {}", exists3.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "KeyExists('greeting') after delete = {}", exists3->value());

  // Verify 'name' is still present.
  auto get2 = db.Get(root, grovedb::Bytes::FromString("name"));
  if (!get2.has_value()) {
    Log(ERROR, "get failed: {}", get2.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO,
      "Get('name') after delete of 'greeting': {} raw bytes (unaffected)",
      get2->value().data().size());

  Log(INFO, "done");
  return EXIT_SUCCESS;
}

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
  Log(INFO, "=== Conditional Insert ===");

  grovedb::test::TempDir dir("ex_conditional_insert");
  auto result = grovedb::Db::Open(dir.PathToString());
  if (!result.has_value()) {
    Log(ERROR, "failed to open database: {}", result.error().message());
    return EXIT_FAILURE;
  }
  auto db = std::move(result).value();
  Log(INFO, "database opened at {}", dir.PathToString());

  grovedb::Path root{};

  // -------------------------------------------------------------------------
  // 1. PutIfAbsent — first insert succeeds, second skips.
  // -------------------------------------------------------------------------

  Log(INFO, "--- PutIfAbsent ---");

  auto pia1 =
      grovedb::Element::Item(grovedb::Bytes::FromString("first")).and_then([&](grovedb::Element e) {
        return db.PutIfAbsent(root, grovedb::Bytes::FromString("unique"), e);
      });
  if (!pia1.has_value()) {
    Log(ERROR, "PutIfAbsent failed: {}", pia1.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "PutIfAbsent('unique', 'first') -> inserted={}", pia1->value());

  auto pia2 = grovedb::Element::Item(grovedb::Bytes::FromString("second"))
                  .and_then([&](grovedb::Element e) {
                    return db.PutIfAbsent(root, grovedb::Bytes::FromString("unique"), e);
                  });
  if (!pia2.has_value()) {
    Log(ERROR, "PutIfAbsent failed: {}", pia2.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "PutIfAbsent('unique', 'second') -> inserted={} (key already exists)", pia2->value());

  // -------------------------------------------------------------------------
  // 2. PutIfAbsentAndGet — returns nullopt on fresh insert.
  // -------------------------------------------------------------------------

  Log(INFO, "--- PutIfAbsentAndGet ---");

  auto piag1 = grovedb::Element::Item(grovedb::Bytes::FromString("brand_new"))
                   .and_then([&](grovedb::Element e) {
                     return db.PutIfAbsentAndGet(root, grovedb::Bytes::FromString("fresh"), e);
                   });
  if (!piag1.has_value()) {
    Log(ERROR, "PutIfAbsentAndGet failed: {}", piag1.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO,
      "PutIfAbsentAndGet('fresh') -> previous has_value={} (inserted, no previous)",
      piag1->value().has_value());

  // PutIfAbsentAndGet on existing key returns the existing element.
  auto piag2 = grovedb::Element::Item(grovedb::Bytes::FromString("try_again"))
                   .and_then([&](grovedb::Element e) {
                     return db.PutIfAbsentAndGet(root, grovedb::Bytes::FromString("fresh"), e);
                   });
  if (!piag2.has_value()) {
    Log(ERROR, "PutIfAbsentAndGet failed: {}", piag2.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO,
      "PutIfAbsentAndGet('fresh') -> previous has_value={} (key exists, returned previous)",
      piag2->value().has_value());

  // -------------------------------------------------------------------------
  // 3. PutIfChanged — conditional update based on value comparison.
  // -------------------------------------------------------------------------

  Log(INFO, "--- PutIfChanged ---");

  auto elem_v1 = grovedb::Element::Item(grovedb::Bytes::FromString("version_1"));
  if (!elem_v1.has_value()) {
    Log(ERROR, "failed to create element: {}", elem_v1.error().message());
    return EXIT_FAILURE;
  }

  // New key — always changed.
  auto pic1 = db.PutIfChanged(root, grovedb::Bytes::FromString("config"), *elem_v1);
  if (!pic1.has_value()) {
    Log(ERROR, "PutIfChanged failed: {}", pic1.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO,
      "PutIfChanged('config', 'version_1') -> changed={}, previous has_value={}",
      pic1->value().m_changed,
      pic1->value().m_previous.has_value());

  // Same value — not changed.
  auto pic2 = db.PutIfChanged(root, grovedb::Bytes::FromString("config"), *elem_v1);
  if (!pic2.has_value()) {
    Log(ERROR, "PutIfChanged failed: {}", pic2.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO,
      "PutIfChanged('config', 'version_1') -> changed={} (same value, no write)",
      pic2->value().m_changed);

  // Different value — changed.
  auto pic3 = grovedb::Element::Item(grovedb::Bytes::FromString("version_2"))
                  .and_then([&](grovedb::Element e) {
                    return db.PutIfChanged(root, grovedb::Bytes::FromString("config"), e);
                  });
  if (!pic3.has_value()) {
    Log(ERROR, "PutIfChanged failed: {}", pic3.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO,
      "PutIfChanged('config', 'version_2') -> changed={}, previous has_value={}",
      pic3->value().m_changed,
      pic3->value().m_previous.has_value());

  Log(INFO, "done");
  return EXIT_SUCCESS;
}

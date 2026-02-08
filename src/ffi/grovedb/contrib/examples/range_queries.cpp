// Copyright (c) 2026-present, The Dash Core developers
// Copyright (c) 2017-present, Cosmos Labs
// Distributed under the Apache 2.0 license, see the accompanying
// file LICENSE.APACHE2 or https://opensource.org/license/apache-2-0
//
// Adapted from IAVL's iterator_test.go

#include <test/util/tempdir.h>

#include <grovedb/db.h>
#include <grovedb/element.h>
#include <grovedb/query.h>

#include <util/logging.h>

#include <cstdlib>
#include <string>

namespace {
/** Run a query and log results. */
bool RunQuery(
    grovedb::Db& db,
    const char* label,
    const std::vector<grovedb::QueryItem>& items,
    uint32_t limit = 0,
    uint32_t offset = 0
)
{
  grovedb::Path root{};
  auto result =
      grovedb::PathQuery::New(root, items, limit, offset).and_then([&](grovedb::PathQuery q) {
        return db.QueryValues(q);
      });
  if (!result.has_value()) {
    Log(ERROR, "QueryValues failed for {}: {}", label, result.error().message());
    return false;
  }

  std::string keys;
  for (const auto& v : result->value().m_data) {
    if (!keys.empty()) {
      keys += ", ";
    }
    keys += v.ToString();
  }
  Log(INFO,
      "{}: {} results [{}], skipped={}",
      label,
      result->value().m_data.size(),
      keys,
      result->value().m_skipped);
  return true;
}
} // anonymous namespace

int main()
{
  Log(INFO, "=== Range Queries ===");

  grovedb::test::TempDir dir("ex_range_queries");
  auto result = grovedb::Db::Open(dir.PathToString());
  if (!result.has_value()) {
    Log(ERROR, "failed to open database: {}", result.error().message());
    return EXIT_FAILURE;
  }
  auto db = std::move(result).value();
  Log(INFO, "database opened at {}", dir.PathToString());

  grovedb::Path root{};

  // -------------------------------------------------------------------------
  // 1. Insert keys "a" through "j" (10 items).
  // -------------------------------------------------------------------------

  Log(INFO, "--- inserting keys a..j ---");
  for (char c = 'a'; c <= 'j'; ++c) {
    auto key = grovedb::Bytes::FromString(std::string(1, c));
    auto cost = grovedb::Element::Item(key).and_then([&](grovedb::Element e) {
      return db.Put(root, key, e);
    });
    if (!cost.has_value()) {
      Log(ERROR, "put failed: {}", cost.error().message());
      return EXIT_FAILURE;
    }
  }
  Log(INFO, "inserted 10 items (a through j)");

  // -------------------------------------------------------------------------
  // 2. Demonstrate all 10 QueryItem factories.
  // -------------------------------------------------------------------------

  Log(INFO, "--- QueryItem factories ---");

  // Key("e") → 1 result
  if (!RunQuery(db, "Key('e')", {grovedb::QueryItem::Key({'e'})})) {
    return EXIT_FAILURE;
  }

  // RangeFull() → 10 results
  if (!RunQuery(db, "RangeFull()", {grovedb::QueryItem::RangeFull()})) {
    return EXIT_FAILURE;
  }

  // Range("c","g") → c,d,e,f
  if (!RunQuery(db, "Range('c','g')", {grovedb::QueryItem::Range({'c'}, {'g'})})) {
    return EXIT_FAILURE;
  }

  // RangeInclusive("c","g") → c,d,e,f,g
  if (!RunQuery(
          db, "RangeInclusive('c','g')", {grovedb::QueryItem::RangeInclusive({'c'}, {'g'})}
      )) {
    return EXIT_FAILURE;
  }

  // RangeFrom("f") → f,g,h,i,j
  if (!RunQuery(db, "RangeFrom('f')", {grovedb::QueryItem::RangeFrom({'f'})})) {
    return EXIT_FAILURE;
  }

  // RangeTo("d") → a,b,c
  if (!RunQuery(db, "RangeTo('d')", {grovedb::QueryItem::RangeTo({'d'})})) {
    return EXIT_FAILURE;
  }

  // RangeToInclusive("d") → a,b,c,d
  if (!RunQuery(db, "RangeToInclusive('d')", {grovedb::QueryItem::RangeToInclusive({'d'})})) {
    return EXIT_FAILURE;
  }

  // RangeAfter("c") → d,e,f,g,h,i,j
  if (!RunQuery(db, "RangeAfter('c')", {grovedb::QueryItem::RangeAfter({'c'})})) {
    return EXIT_FAILURE;
  }

  // RangeAfterTo("b","f") → c,d,e
  if (!RunQuery(db, "RangeAfterTo('b','f')", {grovedb::QueryItem::RangeAfterTo({'b'}, {'f'})})) {
    return EXIT_FAILURE;
  }

  // RangeAfterToInclusive("b","f") → c,d,e,f
  if (!RunQuery(
          db,
          "RangeAfterToInclusive('b','f')",
          {grovedb::QueryItem::RangeAfterToInclusive({'b'}, {'f'})}
      )) {
    return EXIT_FAILURE;
  }

  // -------------------------------------------------------------------------
  // 3. Limit + offset: RangeFull, limit=3, offset=2 → c,d,e.
  // -------------------------------------------------------------------------

  Log(INFO, "--- limit and offset ---");
  if (!RunQuery(
          db,
          "RangeFull(limit=3, offset=2)",
          {grovedb::QueryItem::RangeFull()},
          /*limit=*/3,
          /*offset=*/2
      )) {
    return EXIT_FAILURE;
  }

  Log(INFO, "done");
  return EXIT_SUCCESS;
}

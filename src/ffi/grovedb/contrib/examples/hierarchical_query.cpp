// Copyright (c) 2026-present, The Dash Core developers
// Copyright (c) 2017-present, Dash Core Group, Inc.
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit
//
// Adapted from drive-sdk's hierarchical query patterns

#include <test/util/tempdir.h>

#include <grovedb/db.h>
#include <grovedb/element.h>
#include <grovedb/query.h>

#include <util/logging.h>

#include <cstdlib>
#include <string>

int main()
{
  Log(INFO, "=== Hierarchical Query ===");

  grovedb::test::TempDir dir("ex_hierarchical_query");
  auto result = grovedb::Db::Open(dir.PathToString());
  if (!result.has_value()) {
    Log(ERROR, "failed to open database: {}", result.error().message());
    return EXIT_FAILURE;
  }
  auto db = std::move(result).value();
  Log(INFO, "database opened at {}", dir.PathToString());

  grovedb::Path root{};

  // -------------------------------------------------------------------------
  // 1. Create: root → "s1" and "s2" subtrees, each with items "x" and "y".
  // -------------------------------------------------------------------------

  Log(INFO, "--- creating subtree structure ---");
  for (const char* sub : {"s1", "s2"}) {
    auto c = grovedb::Element::EmptyTree().and_then([&](grovedb::Element e) {
      return db.Put(root, grovedb::Bytes::FromString(sub), e);
    });
    if (!c.has_value()) {
      Log(ERROR, "put subtree failed: {}", c.error().message());
      return EXIT_FAILURE;
    }
    Log(INFO, "created subtree '{}'", sub);

    grovedb::Path sub_path{grovedb::Bytes::FromString(sub)};
    for (const char* key : {"x", "y"}) {
      auto val = grovedb::Bytes::FromString(std::string(sub) + "_" + key);
      auto ck = grovedb::Element::Item(val).and_then([&](grovedb::Element e) {
        return db.Put(sub_path, grovedb::Bytes::FromString(key), e);
      });
      if (!ck.has_value()) {
        Log(ERROR, "put failed: {}", ck.error().message());
        return EXIT_FAILURE;
      }
      Log(INFO, "  inserted '{}' = '{}'", key, val.ToString());
    }
  }

  // -------------------------------------------------------------------------
  // 2. PathQuery::NewWithSubquery — query "x" from all subtrees.
  // -------------------------------------------------------------------------

  Log(INFO, "--- NewWithSubquery: query 'x' from all subtrees ---");

  grovedb::Path empty_subquery_path{};
  auto results = grovedb::PathQuery::NewWithSubquery(
                     root,
                     {grovedb::QueryItem::RangeFull()},
                     /*limit=*/0,
                     /*offset=*/0,
                     empty_subquery_path,
                     {grovedb::QueryItem::Key({'x'})}
  )
                     .and_then([&](grovedb::PathQuery q) { return db.QueryValues(q); });
  if (!results.has_value()) {
    Log(ERROR, "QueryValues failed: {}", results.error().message());
    return EXIT_FAILURE;
  }

  Log(INFO, "subquery returned {} results", results->value().m_data.size());
  for (const auto& v : results->value().m_data) {
    Log(INFO, "  value='{}'", v.ToString());
  }

  // -------------------------------------------------------------------------
  // 3. Compare with equivalent flat queries.
  // -------------------------------------------------------------------------

  Log(INFO, "--- equivalent flat queries for comparison ---");
  for (const char* sub : {"s1", "s2"}) {
    grovedb::Path sub_path{grovedb::Bytes::FromString(sub)};
    auto fr = grovedb::PathQuery::New(sub_path, {grovedb::QueryItem::Key({'x'})})
                  .and_then([&](grovedb::PathQuery q) { return db.QueryValues(q); });
    if (!fr.has_value()) {
      Log(ERROR, "QueryValues failed: {}", fr.error().message());
      return EXIT_FAILURE;
    }
    for (const auto& v : fr->value().m_data) {
      Log(INFO, "  flat query at '{}': value='{}'", sub, v.ToString());
    }
  }
  Log(INFO, "subquery achieves the same results in a single call");

  Log(INFO, "done");
  return EXIT_SUCCESS;
}

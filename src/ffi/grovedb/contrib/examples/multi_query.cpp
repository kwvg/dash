// Copyright (c) 2026-present, The Dash Core developers
// Copyright (c) 2017-present, Dash Core Group, Inc.
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit
//
// Adapted from drive-sdk's multi-query patterns

#include <test/util/tempdir.h>

#include <grovedb/db.h>
#include <grovedb/element.h>
#include <grovedb/query.h>

#include <util/logging.h>

#include <array>
#include <cstdlib>
#include <string>

int main()
{
  Log(INFO, "=== Multi Query ===");

  grovedb::test::TempDir dir("ex_multi_query");
  auto result = grovedb::Db::Open(dir.PathToString());
  if (!result.has_value()) {
    Log(ERROR, "failed to open database: {}", result.error().message());
    return EXIT_FAILURE;
  }
  auto db = std::move(result).value();
  Log(INFO, "database opened at {}", dir.PathToString());

  grovedb::Path root{};

  // -------------------------------------------------------------------------
  // 1. Insert "x", "y", "z" at root.
  // -------------------------------------------------------------------------

  for (const char* k : {"x", "y", "z"}) {
    auto c =
        grovedb::Element::Item(grovedb::Bytes::FromString(k)).and_then([&](grovedb::Element e) {
          return db.Put(root, grovedb::Bytes::FromString(k), e);
        });
    if (!c.has_value()) {
      Log(ERROR, "put failed: {}", c.error().message());
      return EXIT_FAILURE;
    }
    Log(INFO, "inserted '{}'", k);
  }

  // -------------------------------------------------------------------------
  // 2. QueryManyRaw with two RawQuerySpec objects.
  // -------------------------------------------------------------------------

  Log(INFO, "--- QueryManyRaw ---");
  std::vector<grovedb::QueryItem> items1{grovedb::QueryItem::Key({'x'})};
  std::vector<grovedb::QueryItem> items2{grovedb::QueryItem::Key({'z'})};

  std::vector<grovedb::Db::RawQuerySpec> specs{
      {root, items1, /*limit=*/0, /*offset=*/0},
      {root, items2, /*limit=*/0, /*offset=*/0},
  };

  auto many = db.QueryManyRaw(specs, /*result_type=*/1); // KeyElementPair
  if (!many.has_value()) {
    Log(ERROR, "QueryManyRaw failed: {}", many.error().message());
    return EXIT_FAILURE;
  }

  Log(INFO, "QueryManyRaw returned {} results", many->value().size());
  for (const auto& entry : many->value()) {
    Log(INFO, "  kind={}, key='{}'", ToString(entry.m_kind), entry.m_key.ToString());
  }

  // -------------------------------------------------------------------------
  // 3. QueryRaw with result_type=0 (Element), =1 (KeyElement), =2 (PathKeyElement).
  // -------------------------------------------------------------------------

  for (uint8_t rt = 0; rt <= 2; ++rt) {
    const std::array names = {"Element", "KeyElementPair", "PathKeyElementTrio"};
    Log(INFO, "--- QueryRaw result_type={} ({}) ---", rt, names[rt]);

    auto raw = grovedb::PathQuery::New(root, {grovedb::QueryItem::RangeFull()})
                   .and_then([&](grovedb::PathQuery q) { return db.QueryRaw(q, rt); });
    if (!raw.has_value()) {
      Log(ERROR, "QueryRaw failed: {}", raw.error().message());
      return EXIT_FAILURE;
    }

    Log(INFO, "QueryRaw returned {} results", raw->value().m_data.size());
    for (const auto& entry : raw->value().m_data) {
      if (rt == 0) {
        Log(INFO, "  element: {} raw bytes", entry.m_element.data().size());
      } else if (rt == 1) {
        Log(INFO,
            "  key='{}', element: {} raw bytes",
            entry.m_key.ToString(),
            entry.m_element.data().size());
      } else {
        std::string path_str;
        for (const auto& seg : entry.m_path) {
          if (!path_str.empty()) {
            path_str += "/";
          }
          path_str += seg.ToString();
        }
        Log(INFO,
            "  path=[{}], key='{}', element: {} raw bytes",
            path_str,
            entry.m_key.ToString(),
            entry.m_element.data().size());
      }
    }
  }

  // -------------------------------------------------------------------------
  // 4. QueryKeysOptional with existing + missing keys.
  // -------------------------------------------------------------------------

  Log(INFO, "--- QueryKeysOptional ---");
  auto kopt = grovedb::PathQuery::New(
                  root,
                  {grovedb::QueryItem::Key({'x'}),
                   grovedb::QueryItem::Key(grovedb::Bytes::FromString("missing"))},
                  /*limit=*/100
  )
                  .and_then([&](grovedb::PathQuery q) { return db.QueryKeysOptional(q); });
  if (!kopt.has_value()) {
    Log(ERROR, "QueryKeysOptional failed: {}", kopt.error().message());
    return EXIT_FAILURE;
  }

  Log(INFO, "QueryKeysOptional returned {} entries", kopt->value().size());
  for (const auto& entry : kopt->value()) {
    Log(
        INFO, "  key='{}', element present={}", entry.m_key.ToString(), entry.m_element.has_value()
    );
  }

  // -------------------------------------------------------------------------
  // 5. QueryRawKeysOptional — same but without reference following.
  // -------------------------------------------------------------------------

  Log(INFO, "--- QueryRawKeysOptional ---");
  auto rkopt = grovedb::PathQuery::New(
                   root,
                   {grovedb::QueryItem::Key({'y'}),
                    grovedb::QueryItem::Key(grovedb::Bytes::FromString("absent"))},
                   /*limit=*/100
  )
                   .and_then([&](grovedb::PathQuery q) { return db.QueryRawKeysOptional(q); });
  if (!rkopt.has_value()) {
    Log(ERROR, "QueryRawKeysOptional failed: {}", rkopt.error().message());
    return EXIT_FAILURE;
  }

  Log(INFO, "QueryRawKeysOptional returned {} entries", rkopt->value().size());
  for (const auto& entry : rkopt->value()) {
    Log(
        INFO, "  key='{}', element present={}", entry.m_key.ToString(), entry.m_element.has_value()
    );
  }

  Log(INFO, "done");
  return EXIT_SUCCESS;
}

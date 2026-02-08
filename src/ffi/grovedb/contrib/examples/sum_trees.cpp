// Copyright (c) 2026-present, The Dash Core developers
// Copyright (c) 2017-present, Dash Core Group, Inc.
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit
//
// Adapted from drive-sdk's balance tracking patterns

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
  Log(INFO, "=== Sum Trees ===");

  grovedb::test::TempDir dir("ex_sum_trees");
  auto result = grovedb::Db::Open(dir.PathToString());
  if (!result.has_value()) {
    Log(ERROR, "failed to open database: {}", result.error().message());
    return EXIT_FAILURE;
  }
  auto db = std::move(result).value();
  Log(INFO, "database opened at {}", dir.PathToString());

  grovedb::Path root{};

  // -------------------------------------------------------------------------
  // 1. Create a sum tree at root/"balances".
  // -------------------------------------------------------------------------

  auto ct = grovedb::Element::EmptySumTree().and_then([&](grovedb::Element e) {
    return db.Put(root, grovedb::Bytes::FromString("balances"), e);
  });
  if (!ct.has_value()) {
    Log(ERROR, "put EmptySumTree failed: {}", ct.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "created sum tree at root/'balances'");

  // -------------------------------------------------------------------------
  // 2. Insert SumItems: alice=1000, bob=500, charlie=-200.
  // -------------------------------------------------------------------------

  grovedb::Path balances{grovedb::Bytes::FromString("balances")};

  struct Account {
    const char* name;
    int64_t amount;
  };
  std::array<Account, 3> accounts = {{
      {"alice", 1000},
      {"bob", 500},
      {"charlie", -200},
  }};

  for (const auto& acct : accounts) {
    auto c = grovedb::Element::SumItem(acct.amount).and_then([&](grovedb::Element e) {
      return db.Put(balances, grovedb::Bytes::FromString(acct.name), e);
    });
    if (!c.has_value()) {
      Log(ERROR, "put SumItem failed for {}: {}", acct.name, c.error().message());
      return EXIT_FAILURE;
    }
    Log(INFO, "inserted SumItem('{}', {})", acct.name, acct.amount);
  }

  // -------------------------------------------------------------------------
  // 3. QuerySums — get aggregated sum values.
  // -------------------------------------------------------------------------

  Log(INFO, "--- QuerySums ---");
  auto sums = grovedb::PathQuery::New(balances, {grovedb::QueryItem::RangeFull()})
                  .and_then([&](grovedb::PathQuery q) { return db.QuerySums(q); });
  if (!sums.has_value()) {
    Log(ERROR, "QuerySums failed: {}", sums.error().message());
    return EXIT_FAILURE;
  }

  Log(INFO, "QuerySums returned {} values", sums->value().m_data.size());
  for (size_t i = 0; i < sums->value().m_data.size(); ++i) {
    Log(INFO, "  sum[{}] = {}", i, sums->value().m_data[i]);
  }

  // -------------------------------------------------------------------------
  // 4. QueryItemsOrSums — get kind + value for each entry.
  // -------------------------------------------------------------------------

  Log(INFO, "--- QueryItemsOrSums ---");
  auto ios = grovedb::PathQuery::New(balances, {grovedb::QueryItem::RangeFull()})
                 .and_then([&](grovedb::PathQuery q) { return db.QueryItemsOrSums(q); });
  if (!ios.has_value()) {
    Log(ERROR, "QueryItemsOrSums failed: {}", ios.error().message());
    return EXIT_FAILURE;
  }

  Log(INFO, "QueryItemsOrSums returned {} entries", ios->value().m_data.size());
  for (size_t i = 0; i < ios->value().m_data.size(); ++i) {
    const auto& entry = ios->value().m_data[i];
    Log(INFO, "  [{}] kind={}, sum_value={}", i, ToString(entry.m_kind), entry.m_sum_value);
  }

  Log(INFO, "done");
  return EXIT_SUCCESS;
}

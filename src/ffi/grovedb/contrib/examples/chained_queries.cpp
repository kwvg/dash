// Copyright (c) 2026-present, The Dash Core developers
// Copyright (c) 2017-present, Dash Core Group, Inc.
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit
//
// Adapted from drive-sdk's hierarchical proof patterns

#include <test/util/tempdir.h>

#include <grovedb/db.h>
#include <grovedb/element.h>
#include <grovedb/query.h>

#include <util/logging.h>

#include <cstdlib>
#include <string>

int main()
{
  Log(INFO, "=== Chained Queries ===");

  grovedb::test::TempDir dir("ex_chained_queries");
  auto result = grovedb::Db::Open(dir.PathToString());
  if (!result.has_value()) {
    Log(ERROR, "failed to open database: {}", result.error().message());
    return EXIT_FAILURE;
  }
  auto db = std::move(result).value();
  Log(INFO, "database opened at {}", dir.PathToString());

  grovedb::Path root{};

  // -------------------------------------------------------------------------
  // 1. Create: root → "users" → {"alice","bob"} each with "email" item.
  // -------------------------------------------------------------------------

  Log(INFO, "--- creating structure ---");
  auto cu = grovedb::Element::EmptyTree().and_then([&](grovedb::Element e) {
    return db.Put(root, grovedb::Bytes::FromString("users"), e);
  });
  if (!cu.has_value()) {
    Log(ERROR, "put 'users' failed: {}", cu.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "created 'users' subtree");

  grovedb::Path users_path{grovedb::Bytes::FromString("users")};

  for (const char* name : {"alice", "bob"}) {
    auto ct = grovedb::Element::EmptyTree().and_then([&](grovedb::Element e) {
      return db.Put(users_path, grovedb::Bytes::FromString(name), e);
    });
    if (!ct.has_value()) {
      Log(ERROR, "put '{}' subtree failed: {}", name, ct.error().message());
      return EXIT_FAILURE;
    }

    grovedb::Path user_path{grovedb::Bytes::FromString("users"), grovedb::Bytes::FromString(name)};
    auto email_val = grovedb::Bytes::FromString(std::string(name) + "@example.com");
    auto ce = grovedb::Element::Item(email_val).and_then([&](grovedb::Element e) {
      return db.Put(user_path, grovedb::Bytes::FromString("email"), e);
    });
    if (!ce.has_value()) {
      Log(ERROR, "put email failed: {}", ce.error().message());
      return EXIT_FAILURE;
    }
    Log(INFO, "created {}/email = '{}@example.com'", name, name);
  }

  // -------------------------------------------------------------------------
  // 2. Generate a proof for the outer query (RangeFull on users).
  // -------------------------------------------------------------------------

  Log(INFO, "--- generating proof ---");

  // Build with subquery to get emails for all users.
  grovedb::Path empty_path{};
  auto proof = grovedb::PathQuery::NewWithSubquery(
                   users_path,
                   {grovedb::QueryItem::RangeFull()},
                   /*limit=*/0,
                   /*offset=*/0,
                   empty_path,
                   {grovedb::QueryItem::Key(grovedb::Bytes::FromString("email"))}
  )
                   .and_then([&](grovedb::PathQuery q) { return db.Prove(q); });
  if (!proof.has_value()) {
    Log(ERROR, "Prove failed: {}", proof.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "proof generated: {} bytes", proof->value().size());

  // -------------------------------------------------------------------------
  // 3. Verify with chained queries.
  // -------------------------------------------------------------------------

  Log(INFO, "--- VerifyChainedQueries ---");

  // First query: list all users.
  auto first_query = grovedb::PathQuery::New(users_path, {grovedb::QueryItem::RangeFull()});
  if (!first_query.has_value()) {
    Log(ERROR, "first query failed: {}", first_query.error().message());
    return EXIT_FAILURE;
  }

  // Chained query: get "email" from each user subtree.
  auto chained_query = grovedb::PathQuery::New(
      grovedb::Path{}, {grovedb::QueryItem::Key(grovedb::Bytes::FromString("email"))}
  );
  if (!chained_query.has_value()) {
    Log(ERROR, "chained query failed: {}", chained_query.error().message());
    return EXIT_FAILURE;
  }

  grovedb::PathQuery* chained_ptr = &*chained_query;
  auto chain_result = db.VerifyChainedQueries(proof->value(), *first_query, {chained_ptr});

  if (!chain_result.has_value()) {
    Log(ERROR, "VerifyChainedQueries failed: {}", chain_result.error().message());
    return EXIT_FAILURE;
  }

  // -------------------------------------------------------------------------
  // 4. Log ChainedVerifyResult.
  // -------------------------------------------------------------------------

  Log(INFO, "chained verification succeeded");
  Log(INFO, "root hash: {}", chain_result->value().m_root_hash.ToString());
  Log(INFO, "{} query result sets", chain_result->value().m_query_results.size());

  for (size_t qi = 0; qi < chain_result->value().m_query_results.size(); ++qi) {
    const auto& qr = chain_result->value().m_query_results[qi];
    Log(INFO, "  query[{}]: {} entries", qi, qr.size());
    for (const auto& entry : qr) {
      Log(INFO,
          "    key='{}', element present={}",
          entry.m_key.ToString(),
          entry.m_element.has_value());
    }
  }

  Log(INFO, "done");
  return EXIT_SUCCESS;
}

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
  Log(INFO, "=== Error Handling ===");

  grovedb::test::TempDir dir("ex_error_handling");
  auto result = grovedb::Db::Open(dir.PathToString());
  if (!result.has_value()) {
    Log(ERROR, "failed to open database: {}", result.error().message());
    return EXIT_FAILURE;
  }
  auto db = std::move(result).value();
  Log(INFO, "database opened at {}", dir.PathToString());

  grovedb::Path root{};

  // -------------------------------------------------------------------------
  // 1. Get a non-existent key — demonstrates error inspection.
  // -------------------------------------------------------------------------

  Log(INFO, "--- getting a non-existent key ---");
  auto get_result = db.Get(root, grovedb::Bytes::FromString("missing"));
  Log(INFO, "has_value() = {}", get_result.has_value());
  if (!get_result.has_value()) {
    Log(INFO, "error code:    {}", ToString(get_result.error().code()));
    Log(INFO, "error message: {}", get_result.error().message());
  }

  // -------------------------------------------------------------------------
  // 2. Error factory methods.
  // -------------------------------------------------------------------------

  Log(INFO, "--- error factory methods ---");
  auto custom = grovedb::Error::NotFound("my custom message");
  Log(INFO,
      "Error::NotFound('my custom message') -> code={}, message={}",
      ToString(custom.code()),
      custom.message());

  // -------------------------------------------------------------------------
  // 3. Monadic .and_then() chain on success path.
  //    Put a value, then transform the cost into a descriptive string.
  // -------------------------------------------------------------------------

  Log(INFO, "--- monadic .and_then() on success ---");
  auto elem = grovedb::Element::Item(grovedb::Bytes::FromString("hello"));
  if (!elem.has_value()) {
    Log(ERROR, "failed to create element: {}", elem.error().message());
    return EXIT_FAILURE;
  }

  auto chain_result =
      db.Put(root, grovedb::Bytes::FromString("greeting"), *elem)
          .and_then(
              [](grovedb::OperationCost cost) -> grovedb::Result<std::string, grovedb::Error> {
                return std::format(
                    "put succeeded: {} seeks, {} bytes added",
                    cost.m_seek_count,
                    cost.m_storage_added_bytes
                );
              }
          );

  if (chain_result.has_value()) {
    Log(INFO, "chain result: {}", chain_result.value());
  }

  // -------------------------------------------------------------------------
  // 4. Monadic .and_then() chain on failure path — error propagates.
  // -------------------------------------------------------------------------

  Log(INFO, "--- monadic .and_then() on failure ---");
  auto fail_chain = db.Get(root, grovedb::Bytes::FromString("nonexistent"))
                        .and_then(
                            [](
                                grovedb::Costed<grovedb::Element> ce
                            ) -> grovedb::Result<std::string, grovedb::Error> {
                              return std::format("value has {} bytes", ce.value().data().size());
                            }
                        );

  if (!fail_chain.has_value()) {
    Log(INFO,
        "chain propagated error: {} ({})",
        ToString(fail_chain.error().code()),
        fail_chain.error().message());
  }

  // -------------------------------------------------------------------------
  // 5. .map() pattern — transform the success value.
  // -------------------------------------------------------------------------

  Log(INFO, "--- .map() on Get result ---");
  auto map_result =
      db.Get(root, grovedb::Bytes::FromString("greeting"))
          .map([](grovedb::Costed<grovedb::Element> ce) { return ce.value().data().size(); });

  if (map_result.has_value()) {
    Log(INFO, "mapped result: {} raw bytes", map_result.value());
  }

  // -------------------------------------------------------------------------
  // 6. Successful Put returns has_value() == true.
  // -------------------------------------------------------------------------

  Log(INFO, "--- successful Put ---");
  auto elem2 = grovedb::Element::Item(grovedb::Bytes::FromString("world"));
  if (!elem2.has_value()) {
    Log(ERROR, "failed to create element: {}", elem2.error().message());
    return EXIT_FAILURE;
  }

  auto put_result = db.Put(root, grovedb::Bytes::FromString("name"), *elem2);
  Log(INFO, "Put has_value() = {}", put_result.has_value());
  if (put_result.has_value()) {
    Log(INFO,
        "cost: {} seeks, {} bytes added",
        put_result->m_seek_count,
        put_result->m_storage_added_bytes);
  }

  Log(INFO, "done");
  return EXIT_SUCCESS;
}

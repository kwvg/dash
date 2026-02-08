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
  Log(INFO, "=== Auxiliary Storage ===");

  grovedb::test::TempDir dir("ex_auxiliary");
  auto result = grovedb::Db::Open(dir.PathToString());
  if (!result.has_value()) {
    Log(ERROR, "failed to open database: {}", result.error().message());
    return EXIT_FAILURE;
  }
  auto db = std::move(result).value();
  Log(INFO, "database opened at {}", dir.PathToString());

  // -------------------------------------------------------------------------
  // 1. PutAux / GetAux round trip.
  // -------------------------------------------------------------------------

  Log(INFO, "--- PutAux / GetAux ---");
  auto put1 = db.PutAux(grovedb::Bytes::FromString("schema_version"), {'3'});
  if (!put1.has_value()) {
    Log(ERROR, "PutAux failed: {}", put1.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "PutAux('schema_version', '3'): {} seeks", put1->m_seek_count);

  auto get1 = db.GetAux(grovedb::Bytes::FromString("schema_version"));
  if (!get1.has_value()) {
    Log(ERROR, "GetAux failed: {}", get1.error().message());
    return EXIT_FAILURE;
  }
  if (get1->value().has_value()) {
    Log(INFO,
        "GetAux('schema_version') = '{}'",
        std::string(get1->value()->begin(), get1->value()->end()));
  }

  // -------------------------------------------------------------------------
  // 2. GetAux for nonexistent key → nullopt.
  // -------------------------------------------------------------------------

  Log(INFO, "--- GetAux nonexistent ---");
  auto get2 = db.GetAux(grovedb::Bytes::FromString("nonexistent"));
  if (!get2.has_value()) {
    Log(ERROR, "GetAux failed: {}", get2.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "GetAux('nonexistent') has_value = {}", get2->value().has_value());

  // -------------------------------------------------------------------------
  // 3. PutAux overwrite.
  // -------------------------------------------------------------------------

  Log(INFO, "--- PutAux overwrite ---");
  auto put2 = db.PutAux(grovedb::Bytes::FromString("schema_version"), {'4'});
  if (!put2.has_value()) {
    Log(ERROR, "PutAux overwrite failed: {}", put2.error().message());
    return EXIT_FAILURE;
  }

  auto get3 = db.GetAux(grovedb::Bytes::FromString("schema_version"));
  if (!get3.has_value()) {
    Log(ERROR, "GetAux failed: {}", get3.error().message());
    return EXIT_FAILURE;
  }
  if (get3->value().has_value()) {
    Log(INFO,
        "GetAux('schema_version') after overwrite = '{}'",
        std::string(get3->value()->begin(), get3->value()->end()));
  }

  // -------------------------------------------------------------------------
  // 4. DeleteAux.
  // -------------------------------------------------------------------------

  Log(INFO, "--- DeleteAux ---");
  auto del = db.DeleteAux(grovedb::Bytes::FromString("schema_version"));
  if (!del.has_value()) {
    Log(ERROR, "DeleteAux failed: {}", del.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "DeleteAux('schema_version') succeeded");

  auto get4 = db.GetAux(grovedb::Bytes::FromString("schema_version"));
  if (!get4.has_value()) {
    Log(ERROR, "GetAux failed: {}", get4.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "GetAux('schema_version') after delete has_value = {}", get4->value().has_value());

  // -------------------------------------------------------------------------
  // 5. Aux is separate from Merkle tree — root hash unchanged.
  // -------------------------------------------------------------------------

  Log(INFO, "--- aux vs Merkle tree ---");
  auto hash_before = db.GetRootHash();
  if (!hash_before.has_value()) {
    Log(ERROR, "GetRootHash failed: {}", hash_before.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "root hash before aux write: {}", hash_before->value().ToString());

  auto put3 =
      db.PutAux(grovedb::Bytes::FromString("metadata"), grovedb::Bytes::FromString("some_value"));
  if (!put3.has_value()) {
    Log(ERROR, "PutAux failed: {}", put3.error().message());
    return EXIT_FAILURE;
  }

  auto hash_after = db.GetRootHash();
  if (!hash_after.has_value()) {
    Log(ERROR, "GetRootHash failed: {}", hash_after.error().message());
    return EXIT_FAILURE;
  }
  Log(INFO, "root hash after aux write:  {}", hash_after->value().ToString());
  Log(INFO, "root hash unchanged by aux: {}", hash_before->value() == hash_after->value());

  Log(INFO, "done");
  return EXIT_SUCCESS;
}

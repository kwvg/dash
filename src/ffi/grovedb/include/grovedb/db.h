// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#ifndef LIBGROVEDB_DB_H
#define LIBGROVEDB_DB_H

#include <grovedb/cost.h>
#include <grovedb/costed.h>
#include <grovedb/element.h>
#include <grovedb/error.h>
#include <grovedb/result.h>
#include <grovedb/transaction.h>
#include <grovedb/types.h>

#include <memory>
#include <string>

namespace grovedb {
/**
 * Primary handle to a GroveDB database instance.
 *
 * Move-only. Operations return Result<T, Error> for monadic composition.
 * Phase 0 exposes only the class skeleton; operations are added in later phases.
 */
class Db
{
public:
  Db();
  ~Db();

  Db(const Db&) = delete;
  Db& operator=(const Db&) = delete;
  Db(Db&&);
  Db& operator=(Db&&);

private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
};

/** Return a human-readable build identification string. */
[[nodiscard]] std::string GetWhoami();
} // namespace grovedb

#endif // LIBGROVEDB_DB_H

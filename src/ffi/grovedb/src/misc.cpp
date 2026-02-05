// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <grovedb/grovedb.h>

#include <rust/grovedb_cxx/lib.h>

#include <format>
#include <string>

namespace grovedb {
std::string GetWhoami()
{
  return std::format("libgrovedb uses {}", std::string(grovedb_cxx::whoami()));
}
} // namespace grovedb

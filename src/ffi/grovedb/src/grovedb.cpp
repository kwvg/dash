// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifdef HAVE_GROVEDB_CONFIG
#include <config/grovedb-config.h>
#endif

#include <grovedb/grovedb.h>

#include <rust/grovedb_cxx/lib.h>

#include <format>
#include <string>

namespace grovedb {
std::string GetWhoami()
{
    return std::format("{} {} uses {}", PACKAGE_NAME, PACKAGE_VERSION, std::string(grovedb_cxx::whoami()));
}
} // namespace grovedb

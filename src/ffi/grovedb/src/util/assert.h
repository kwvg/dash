// Copyright (c) 2019-2021, The Bitcoin Core developers
// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GROVEDB_UTIL_ASSERT_H
#define GROVEDB_UTIL_ASSERT_H

#include <util/attributes.h>

#include <cstdio>
#include <cstdlib>
#include <utility>

template <typename T>
T&& inline_assertion_check(LIFETIMEBOUND T&& val, [[maybe_unused]] const char* file, [[maybe_unused]] int line, [[maybe_unused]] const char* func, [[maybe_unused]] const char* cond, [[maybe_unused]] const char* msg)
{
  if (!val) {
    std::fprintf(stderr, "%s:%d %s: Assertion '%s' failed, %s.\n", file, line, func, cond, msg);
    std::fflush(stderr);
    std::abort();
  }
  return std::forward<T>(val);
}

#define Assert(cond, msg) inline_assertion_check(cond, __FILE__, __LINE__, __func__, #cond, #msg)

#endif // GROVEDB_UTIL_ASSERT_H

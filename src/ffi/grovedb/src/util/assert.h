// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GROVEDB_UTIL_ASSERT_H
#define GROVEDB_UTIL_ASSERT_H

#include <cassert>

// Assert with a diagnostic message.  The message is visible in the
// stringified expression that assert() prints when it fires, e.g.:
//
//   Assertion failed: (m_impl) && ("Db::Flush called on uninitialized database")
//
// No need to embed __FILE__ / __LINE__ manually — every conforming
// assert() implementation already prints them.

#define Assert(cond, msg) assert((cond) && (msg))

#endif // GROVEDB_UTIL_ASSERT_H

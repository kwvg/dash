// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#ifndef GROVEDB_UTIL_ATTRIBUTES_H
#define GROVEDB_UTIL_ATTRIBUTES_H

#if defined(__clang__) && __has_attribute(lifetimebound)
#define LIFETIMEBOUND [[clang::lifetimebound]]
#elif defined(_MSC_VER) && __has_attribute(lifetimebound)
#define LIFETIMEBOUND [[msvc::lifetimebound]]
#elif defined(__has_cpp_attribute) && __has_cpp_attribute(lifetimebound)
#define LIFETIMEBOUND [[lifetimebound]]
#else
#define LIFETIMEBOUND
#endif

#endif // GROVEDB_UTIL_ATTRIBUTES_H

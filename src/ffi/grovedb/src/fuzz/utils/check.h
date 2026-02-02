// Adapted from RocksDB fuzz/util.h
// Copyright (c) Meta Platforms, Inc. and affiliates.
// Licensed under GPLv2 and Apache 2.0.
//
// Assertion macros for fuzz harnesses.

#ifndef GROVEDB_FUZZ_UTILS_CHECK_H
#define GROVEDB_FUZZ_UTILS_CHECK_H

#include <cstdlib>
#include <iostream>

#define CHECK_OK(expr)                                      \
    do {                                                    \
        auto s = (expr);                                    \
        if (!s.ok()) {                                      \
            std::cerr << s.message() << std::endl;          \
            std::abort();                                   \
        }                                                   \
    } while (0)

#define CHECK_TRUE(cond)                                    \
    do {                                                    \
        if (!(cond)) {                                      \
            std::cerr << #cond << " is false" << std::endl; \
            std::abort();                                   \
        }                                                   \
    } while (0)

#define CHECK_EQ(a, b)                                      \
    do {                                                    \
        if ((a) != (b)) {                                   \
            std::cerr << #a << " != " << #b << std::endl;   \
            std::abort();                                   \
        }                                                   \
    } while (0)

#endif // GROVEDB_FUZZ_UTILS_CHECK_H

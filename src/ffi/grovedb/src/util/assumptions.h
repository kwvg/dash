// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GROVEDB_UTIL_ASSUMPTIONS_H
#define GROVEDB_UTIL_ASSUMPTIONS_H

#include <climits>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <type_traits>

// Platform assumptions that must hold for the GroveDB FFI wire protocol.
// All checks are compile-time via static_assert.

namespace grovedb::assumptions {

// ============================================================================
// Character and byte assumptions
// ============================================================================

static_assert(CHAR_BIT == 8, "GroveDB requires 8-bit bytes");
static_assert(sizeof(uint8_t) == 1, "uint8_t must be exactly 1 byte");
static_assert(sizeof(char) == 1, "char must be exactly 1 byte");

// ============================================================================
// Integer type size assumptions
// ============================================================================

static_assert(sizeof(uint16_t) == 2, "uint16_t must be 2 bytes");
static_assert(sizeof(uint32_t) == 4, "uint32_t must be 4 bytes");
static_assert(sizeof(uint64_t) == 8, "uint64_t must be 8 bytes");

static_assert(sizeof(int16_t) == 2, "int16_t must be 2 bytes");
static_assert(sizeof(int32_t) == 4, "int32_t must be 4 bytes");
static_assert(sizeof(int64_t) == 8, "int64_t must be 8 bytes");

// ============================================================================
// Two's complement representation (guaranteed in C++20)
// ============================================================================

static_assert(static_cast<int8_t>(-1) == ~static_cast<int8_t>(0),
              "Signed integers must use two's complement");

// ============================================================================
// Pointer and size_t assumptions
// ============================================================================

static_assert(sizeof(size_t) >= sizeof(void*),
              "size_t must be at least as large as a pointer");

static_assert(sizeof(size_t) == 4 || sizeof(size_t) == 8,
              "size_t must be either 4 or 8 bytes");

// ============================================================================
// Wire protocol length encoding assumptions
// ============================================================================

// The wire protocol uses u32 for length encoding, imposing a 4 GiB limit
// per structure.
inline constexpr size_t WIRE_MAX_LENGTH = std::numeric_limits<uint32_t>::max();

static_assert(WIRE_MAX_LENGTH == 0xFFFFFFFFu,
              "Wire protocol length encoding limited to 4 GiB per structure");

// ============================================================================
// std::span assumptions (C++20)
// ============================================================================

static_assert(std::is_same_v<decltype(std::declval<std::span<uint8_t>>().size()), size_t>,
              "std::span::size() must return size_t");

} // namespace grovedb::assumptions

#endif // GROVEDB_UTIL_ASSUMPTIONS_H

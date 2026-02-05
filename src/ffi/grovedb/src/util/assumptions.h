// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#ifndef GROVEDB_UTIL_ASSUMPTIONS_H
#define GROVEDB_UTIL_ASSUMPTIONS_H

#include <climits>
#include <cstdint>
#include <limits>
#include <span>

namespace grovedb {
/** The wire protocol uses u32 for length encoding, imposing a 4 GiB limit per structure. */
inline constexpr size_t WIRE_MAX_LENGTH = std::numeric_limits<uint32_t>::max();

static_assert(CHAR_BIT == 8, "GroveDB requires 8-bit bytes");
static_assert(
    WIRE_MAX_LENGTH == 0xFFFFFFFFu, "Wire protocol length encoding limited to 4 GiB per structure"
);
static_assert(
    std::is_same_v<decltype(std::declval<std::span<uint8_t>>().size()), size_t>,
    "std::span::size() must return size_t"
);
} // namespace grovedb

#endif // GROVEDB_UTIL_ASSUMPTIONS_H

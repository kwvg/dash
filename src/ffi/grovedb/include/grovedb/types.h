// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GROVEDB_TYPES_H
#define GROVEDB_TYPES_H

#include <array>
#include <cstdint>
#include <vector>

namespace grovedb {

/** 32-byte Merkle root hash. */
using Hash = std::array<uint8_t, 32>;

/** Variable-length byte buffer. */
using Bytes = std::vector<uint8_t>;

/**
 * Path to a GroveDB subtree, represented as a sequence of byte segments.
 *
 * Example: `Path{Bytes{'r','o','o','t'}, Bytes{'c','h','i','l','d'}}`
 */
using Path = std::vector<Bytes>;

} // namespace grovedb

#endif // GROVEDB_TYPES_H

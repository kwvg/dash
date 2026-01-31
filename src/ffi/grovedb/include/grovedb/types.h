// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GROVEDB_TYPES_H
#define GROVEDB_TYPES_H

#include <array>
#include <cstdint>

namespace grovedb {

/** 32-byte Merkle root hash. */
using Hash = std::array<uint8_t, 32>;

} // namespace grovedb

#endif // GROVEDB_TYPES_H

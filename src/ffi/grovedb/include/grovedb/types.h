// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LIBGROVEDB_TYPES_H
#define LIBGROVEDB_TYPES_H

#include <cstdint>
#include <vector>

namespace grovedb {
/** A byte vector. */
using Bytes = std::vector<uint8_t>;

/** A path, represented as a vector of byte vectors. */
using Path = std::vector<Bytes>;
} // namespace grovedb

#endif // LIBGROVEDB_TYPES_H

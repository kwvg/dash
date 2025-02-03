// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_COMMON_H
#define BITCOIN_EVO_COMMON_H

#include <cstdint>

enum MnNetStatus : uint8_t
{
    // Adding entries
    Duplicate,
    BadInput,
    BadPort,

    // Removing entries
    NotFound,

    Success
};

#endif // BITCOIN_EVO_COMMON_H

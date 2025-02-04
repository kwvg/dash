// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_COMMON_H
#define BITCOIN_EVO_COMMON_H

#include <serialize.h>

#include <cstdint>
#include <string>

enum MnNetStatus : uint8_t
{
    // Adding entries
    Duplicate,
    BadInput,
    BadPort,
    MaxLimit,

    // Removing entries
    NotFound,

    Success
};

// TODO: Currently this corresponds to the index, is this a good idea?
enum class Purpose : uint8_t
{
    // Mandatory for all masternodes
    CORE_P2P = 0,
    // Mandatory for all EvoNodes
    PLATFORM_P2P = 1,
    // Optional for EvoNodes
    PLATFORM_API = 2
};
template<> struct is_serializable_enum<Purpose> : std::true_type {};

#endif // BITCOIN_EVO_COMMON_H

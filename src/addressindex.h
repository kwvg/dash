// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ADDRESSINDEX_H
#define BITCOIN_ADDRESSINDEX_H

#include <uint256.h>
#include <amount.h>

#include <chrono>

namespace AddressType {
enum AddressType {
    P2PK = 1,
    P2PKH = 1,
    P2SH = 2,

    UNKNOWN = 0
};
}; /* namespace AddressType */

struct CMempoolAddressDelta
{
public:
    std::chrono::seconds m_time;
    CAmount m_amount;
    uint256 m_prev_hash;
    uint32_t m_prev_out{0};

public:
    CMempoolAddressDelta(std::chrono::seconds time, CAmount amount, uint256 prev_hash, uint32_t prev_out) :
        m_time{time}, m_amount{amount}, m_prev_hash{prev_hash}, m_prev_out{prev_out} {}

    CMempoolAddressDelta(std::chrono::seconds time, CAmount amount) :
        m_time{time}, m_amount{amount} {}
};

struct CMempoolAddressDeltaKey
{
public:
    uint8_t m_address_type{AddressType::UNKNOWN};
    uint160 m_address_bytes;
    uint256 m_tx_hash;
    uint32_t m_tx_index{0};
    bool m_tx_spent{false};

public:
    CMempoolAddressDeltaKey(uint8_t address_type, uint160 address_bytes, uint256 tx_hash, uint32_t tx_index, bool tx_spent) :
        m_address_type{address_type},
        m_address_bytes{address_bytes},
        m_tx_hash{tx_hash},
        m_tx_index{tx_index},
        m_tx_spent{tx_spent} {};

    CMempoolAddressDeltaKey(uint8_t address_type, uint160 address_bytes) :
        m_address_type{address_type},
        m_address_bytes{address_bytes} {};
};

struct CMempoolAddressDeltaKeyCompare
{
    bool operator()(const CMempoolAddressDeltaKey& a, const CMempoolAddressDeltaKey& b) const {
        if (a.m_address_type != b.m_address_type) return a.m_address_type < b.m_address_type;
        if (a.m_address_bytes != b.m_address_bytes) return a.m_address_bytes < b.m_address_bytes;
        if (a.m_tx_hash  != b.m_tx_hash)  return a.m_tx_hash  < b.m_tx_hash;
        if (a.m_tx_index != b.m_tx_index) return a.m_tx_index < b.m_tx_index;

        return (a.m_tx_spent < b.m_tx_spent);
    }
};

#endif // BITCOIN_ADDRESSINDEX_H

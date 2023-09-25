// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2016 BitPay, Inc.
// Copyright (c) 2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ADDRESSINDEX_H
#define BITCOIN_ADDRESSINDEX_H

#include <uint256.h>
#include <amount.h>

#include <chrono>

class CScript;

enum class AddressType : uint8_t {
    P2PK = 1,
    P2PKH = 1,
    P2SH = 2,

    UNKNOWN = 0
};

template<typename T> struct is_serializable_enum;
template<> struct is_serializable_enum<AddressType> : std::true_type {};

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
    AddressType m_address_type{AddressType::UNKNOWN};
    uint160 m_address_bytes;
    uint256 m_tx_hash;
    uint32_t m_tx_index{0};
    bool m_tx_spent{false};

public:
    CMempoolAddressDeltaKey(AddressType address_type, uint160 address_bytes, uint256 tx_hash, uint32_t tx_index, bool tx_spent) :
        m_address_type{address_type},
        m_address_bytes{address_bytes},
        m_tx_hash{tx_hash},
        m_tx_index{tx_index},
        m_tx_spent{tx_spent} {};

    CMempoolAddressDeltaKey(AddressType address_type, uint160 address_bytes) :
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

struct CAddressIndexKey {
public:
    AddressType m_address_type{AddressType::UNKNOWN};
    uint160 m_address_bytes;
    int32_t m_block_height{0};
    uint32_t m_block_tx_pos{0};
    uint256 m_tx_hash;
    uint32_t m_tx_index{0};
    bool m_tx_spent{false};

public:
    CAddressIndexKey() {
        SetNull();
    }

    CAddressIndexKey(AddressType address_type, uint160 address_bytes, int32_t block_height, uint32_t block_tx_pos, uint256 tx_hash,
                     uint32_t tx_index, bool tx_spent) :
        m_address_type{address_type},
        m_address_bytes{address_bytes},
        m_block_height{block_height},
        m_block_tx_pos{block_tx_pos},
        m_tx_hash{tx_hash},
        m_tx_index{tx_index},
        m_tx_spent{tx_spent} {};

    void SetNull() {
        m_address_type = AddressType::UNKNOWN;
        m_address_bytes.SetNull();
        m_block_height = 0;
        m_block_tx_pos = 0;
        m_tx_hash.SetNull();
        m_tx_index = 0;
        m_tx_spent = false;
    }

public:
    size_t GetSerializeSize(int nType, int nVersion) const {
        return /* m_address_type */   1 +
               /* m_address_bytes */ 20 +
               /* m_block_height */   4 +
               /* m_block_tx_pos */   4 +
               /* m_tx_hash */       32 +
               /* m_tx_index */       4 +
               /* m_tx_spent */       1;
    }

    template<typename Stream>
    void Serialize(Stream& s) const {
        ser_writedata8(s, static_cast<uint8_t>(m_address_type));
        m_address_bytes.Serialize(s);
        // Heights are stored big-endian for key sorting in LevelDB
        ser_writedata32be(s, m_block_height);
        ser_writedata32be(s, m_block_tx_pos);
        m_tx_hash.Serialize(s);
        ser_writedata32(s, m_tx_index);
        char f = m_tx_spent;
        ser_writedata8(s, f);
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        m_address_type = static_cast<AddressType>(ser_readdata8(s));
        m_address_bytes.Unserialize(s);
        m_block_height = ser_readdata32be(s);
        m_block_tx_pos = ser_readdata32be(s);
        m_tx_hash.Unserialize(s);
        m_tx_index = ser_readdata32(s);
        char f = ser_readdata8(s);
        m_tx_spent = f;
    }
};

struct CAddressIndexIteratorKey {
public:
    AddressType m_address_type{AddressType::UNKNOWN};
    uint160 m_address_bytes;

public:
    CAddressIndexIteratorKey() {
        SetNull();
    }

    CAddressIndexIteratorKey(AddressType address_type, uint160 address_bytes) :
        m_address_type{address_type}, m_address_bytes{address_bytes} {};

    void SetNull() {
        m_address_type = AddressType::UNKNOWN;
        m_address_bytes.SetNull();
    }

public:
    size_t GetSerializeSize(int nType, int nVersion) const {
        return /* m_address_type */  20 +
               /* m_address_bytes */  1;
    }

    template<typename Stream>
    void Serialize(Stream& s) const {
        ser_writedata8(s, static_cast<uint8_t>(m_address_type));
        m_address_bytes.Serialize(s);
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        m_address_type = static_cast<AddressType>(ser_readdata8(s));
        m_address_bytes.Unserialize(s);
    }
};

struct CAddressIndexIteratorHeightKey {
public:
    AddressType m_address_type{AddressType::UNKNOWN};
    uint160 m_address_bytes;
    int32_t m_block_height{0};

public:
    CAddressIndexIteratorHeightKey() {
        SetNull();
    }

    CAddressIndexIteratorHeightKey(AddressType address_type, uint160 address_bytes, int32_t block_height) :
        m_address_type{address_type}, m_address_bytes{address_bytes}, m_block_height{block_height} {};

    void SetNull() {
        m_address_type = AddressType::UNKNOWN;
        m_address_bytes.SetNull();
        m_block_height = 0;
    }

public:
    size_t GetSerializeSize(int nType, int nVersion) const {
        return /* m_address_type */   1 +
               /* m_address_bytes */ 20 +
               /* m_block_height */   4;
    }

    template<typename Stream>
    void Serialize(Stream& s) const {
        ser_writedata8(s, static_cast<uint8_t>(m_address_type));
        m_address_bytes.Serialize(s);
        ser_writedata32be(s, m_block_height);
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        m_address_type = static_cast<AddressType>(ser_readdata8(s));
        m_address_bytes.Unserialize(s);
        m_block_height = ser_readdata32be(s);
    }
};

bool AddressBytesFromScript(const CScript& script, uint8_t& address_type, uint160& address_bytes);

#endif // BITCOIN_ADDRESSINDEX_H

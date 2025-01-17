// Copyright (c) 2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MASTERNODE_ADDRESS_H
#define BITCOIN_MASTERNODE_ADDRESS_H

#include <uint256.h>

#include <string>

// MnAddr is a bech32m encoded masternode ProTx hash that can be used
// to identify and interact with a masternode.
class MnAddr
{
private:
    const uint256 protx_hash;
    const bool is_valid;

public:
    MnAddr() = delete;
    ~MnAddr() = default;

    MnAddr(uint256 hash) : protx_hash{hash}, is_valid{true} {};
    MnAddr(std::string addr, std::string& error);

    // Get the validity of the MnAddr
    bool IsValid() const { return is_valid; }
    // Get the bech32-encoded address of the collateral
    std::string GetAddress() const;
    // Get the collateral hash from a bech32-encoded address
    uint256 GetHash() const { return protx_hash; }
};

#endif // BITCOIN_MASTERNODE_ADDRESS_H

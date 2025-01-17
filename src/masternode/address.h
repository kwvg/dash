// Copyright (c) 2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MASTERNODE_ADDRESS_H
#define BITCOIN_MASTERNODE_ADDRESS_H

#include <uint256.h>

#include <optional>
#include <string>

class CDeterministicMNManager;
class CService;

// MnAddr is a bech32m encoded masternode ProTx hash that can be used
// to identify and interact with a masternode.
class MnAddr
{
private:
    const uint256 protx_hash;
    const bool is_valid;
    const std::string address;

public:
    // Error codes when unable to decode an MnAddr string
    enum class DecodeStatus : uint8_t
    {
        NotBech32m,
        HRPBad,
        DataEmpty,
        DataVersionBad,
        DataPaddingBad,
        DataSizeBad,

        Success
    };

public:
    MnAddr() = delete;
    ~MnAddr();

    MnAddr(uint256 hash);
    MnAddr(std::string addr, MnAddr::DecodeStatus& status);

    // Get the validity of the MnAddr
    bool IsValid() const { return is_valid; }
    // Get the bech32-encoded address of the collateral
    const std::string& GetAddress() const { return address; }
    // Get the collateral hash from a bech32-encoded address
    const uint256& GetHash() const { return protx_hash; }
};

// Converts DecodeStatus to human-readable error
std::string DSToString(MnAddr::DecodeStatus status);

// Tries to find the connection details registered for a masternode by the collateral hash encoded within a given MnAddr
std::optional<CService> GetConnectionDetails(CDeterministicMNManager& dmnman, const MnAddr mn_addr, std::string& error_str);

#endif // BITCOIN_MASTERNODE_ADDRESS_H

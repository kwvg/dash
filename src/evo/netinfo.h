// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_NETINFO_H
#define BITCOIN_EVO_NETINFO_H

#include <netaddress.h>
#include <serialize.h>

class CDeterministicMN;
class CDeterministicMNState;
class CProRegTx;
class CProUpServTx;
class CService;
class CSimplifiedMNListEntry;
enum class MnType : uint16_t;

class UniValue;

enum NetInfoStatus : uint8_t
{
    // Adding entries
    Duplicate,
    MaxLimit,

    // Validation
    BadInput,
    BadPort,
    Success
};

constexpr std::string_view NISToString(const NetInfoStatus code) {
    switch (code) {
    case NetInfoStatus::Duplicate:
        return "duplicate";
    case NetInfoStatus::MaxLimit:
        return "too many entries";
    case NetInfoStatus::BadInput:
        return "invalid network address";
    case NetInfoStatus::BadPort:
        return "invalid port";
    case NetInfoStatus::Success:
        return "success";
    } // no default case, so the compiler can warn about missing cases
}

enum class Purpose : uint8_t
{
    // Mandatory for masternodes
    CORE_P2P = 0,
    // Mandatory for EvoNodes
    PLATFORM_P2P = 1,
    // Optional for EvoNodes
    PLATFORM_HTTP = 2,
};
template<> struct is_serializable_enum<Purpose> : std::true_type {};

// Warning: Used in RPC code, altering existing values is a breaking change
constexpr std::string PurposeToString(const Purpose code, const bool lower = false) {
    switch (code) {
    case Purpose::CORE_P2P:
        return lower ? "core_p2p" : "CORE_P2P";
    case Purpose::PLATFORM_HTTP:
        return lower ? "platform_http" : "PLATFORM_HTTP";
    case Purpose::PLATFORM_P2P:
        return lower ? "platform_p2p" : "PLATFORM_P2P";
    } // no default case, so the compiler can warn about missing cases
}

using CServiceList = std::vector<std::reference_wrapper<const CService>>;

class MnNetInfo
{
private:
    CService addr;

private:
    static NetInfoStatus ValidateService(const CService& service);

public:
    MnNetInfo() = default;
    ~MnNetInfo() = default;

    bool operator==(const MnNetInfo& rhs) const { return addr == rhs.addr; }
    bool operator!=(const MnNetInfo& rhs) const { return !(*this == rhs); }

    SERIALIZE_METHODS(MnNetInfo, obj)
    {
        READWRITE(obj.addr);
    }

    NetInfoStatus AddEntry(const Purpose purpose, const std::string& input);
    CServiceList GetEntries() const;

    const CService& GetPrimary() const { return addr; }
    bool IsEmpty() const { return *this == MnNetInfo(); }
    NetInfoStatus Validate() const { return ValidateService(addr); }
    UniValue ToJson() const;
    std::string ToString() const;

    void Clear() { addr = CService(); }
};

/* Wraps a CService::ToStringAddrPort() into a UniValue array */
UniValue ArrFromService(const CService& addr);

/* Identical to IsDeprecatedRPCEnabled("service"). For use outside of RPC code. */
bool IsServiceDeprecatedRPCEnabled();

/* Populates a MnNetInfo::GetJson() output with platform network info. */
UniValue MaybeAddPlatformNetInfo(const CDeterministicMN& dmn, const UniValue& arr);
UniValue MaybeAddPlatformNetInfo(const CDeterministicMNState& obj, const MnType& type, const UniValue& arr);
UniValue MaybeAddPlatformNetInfo(const CProRegTx& obj, const UniValue& arr);
UniValue MaybeAddPlatformNetInfo(const CProUpServTx& obj, const UniValue& arr);
UniValue MaybeAddPlatformNetInfo(const CSimplifiedMNListEntry& obj, const MnType& type, const UniValue& arr);

#endif // BITCOIN_EVO_NETINFO_H

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

// Warning: Used in RPC code, altering existing values is a breaking change
constexpr std::string PurposeToString(const Purpose code) {
    switch (code) {
    case Purpose::CORE_P2P:
        return "CORE_P2P";
    case Purpose::PLATFORM_HTTP:
        return "PLATFORM_HTTP";
    case Purpose::PLATFORM_P2P:
        return "PLATFORM_P2P";
    } // no default case, so the compiler can warn about missing cases
}

namespace {
uint8_t GetBIP155FromService(const CService& service)
{
    if (service.IsIPv4())  return 0x01 /* BIP155Network::IPV4 */;
    if (service.IsIPv6())  return 0x02 /* BIP155Network::IPV6 */;
    if (service.IsTor())   return 0x04 /* BIP155Network::TORV3 */;
    if (service.IsI2P())   return 0x05 /* BIP155Network::I2P */;
    if (service.IsCJDNS()) return 0x06 /* BIP155Network::CJDNS */;
    return 0xFF; /* invalid type */
}

bool IsTypeBIP155(const uint8_t type)
{
    switch (type) {
    case 0x01: /* BIP155Network::IPV4 */
    case 0x02: /* BIP155Network::IPV6 */
    case 0x04: /* BIP155Network::TORV3 */
    case 0x05: /* BIP155Network::I2P */
    case 0x06: /* BIP155Network::CJDNS */
        return true;
    default:
        return false;
    }
}
} // anonymous namespace

class NetInfo
{
private:
    uint8_t type;
    CNetAddr addr;
    uint16_t port;

public:
    NetInfo() = default;
    ~NetInfo() = default;

    NetInfo(CService service) : type{GetBIP155FromService(service)}, addr{service}, port{service.GetPort()} {}

    bool operator==(const NetInfo& rhs) const { return type == rhs.type && addr == rhs.addr && port == rhs.port; }
    bool operator!=(const NetInfo& rhs) const { return !(*this == rhs); }
    bool operator<(const NetInfo& rhs) const { return std::tie(addr, port) < std::tie(rhs.addr, rhs.port); }

    template<typename Stream>
    void Serialize(Stream &s) const
    {
        s << type;
        if (IsTypeBIP155(type)) {
            s << addr;
        } else {
            // Invalid type, bail out
            return;
        }
        s << port;
    }

    template<typename Stream>
    void Unserialize(Stream &s)
    {
        Clear();
        s >> type;
        if (IsTypeBIP155(type)) {
            s >> addr;
        } else {
            // Invalid type, bail out
            return;
        }
        s >> port;
    }

    void Clear()
    {
        type = 0;
        addr = CNetAddr();
        port = 0;
    }

    std::optional<CService> GetAddrPort() const;
    std::string ToString() const;
};

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

    NetInfoStatus AddEntry(const Purpose purpose, const std::string input);
    std::vector<NetInfo> GetEntries() const;

    const CService& GetPrimary() const { return addr; }
    bool IsEmpty() const { return *this == MnNetInfo(); }
    NetInfoStatus Validate() const { return ValidateService(addr); }
    UniValue ToJson() const;
    std::string ToString() const;

    void Clear() { addr = CService(); }
};

/* Wraps a CService::ToStringAddrPort() into a UniValue array */
UniValue ArrFromService(CService addr);

/* Identical to IsDeprecatedRPCEnabled("service"). For use outside of RPC code. */
bool IsServiceDeprecatedRPCEnabled();

/* Populates a MnNetInfo::GetJson() output with platform network info. */
UniValue MaybeAddPlatformNetInfo(const CDeterministicMN& dmn, UniValue arr);
UniValue MaybeAddPlatformNetInfo(const CDeterministicMNState& obj, MnType type, UniValue arr);
UniValue MaybeAddPlatformNetInfo(const CProRegTx& obj, UniValue arr);
UniValue MaybeAddPlatformNetInfo(const CProUpServTx& obj, UniValue arr);
UniValue MaybeAddPlatformNetInfo(const CSimplifiedMNListEntry& obj, MnType type, UniValue arr);

#endif // BITCOIN_EVO_NETINFO_H

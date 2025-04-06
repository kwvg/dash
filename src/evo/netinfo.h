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

static constexpr uint8_t EXTNETINFO_ENTRIES_LIMIT{32};

enum NetInfoStatus : uint8_t
{
    // Adding entries
    Duplicate,
    MaxLimit,

    // Validation
    BadInput,
    BadPort,
    Malformed,
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
    case NetInfoStatus::Malformed:
        return "malformed";
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

inline constexpr bool IsValidPurpose(uint8_t purpose)
{
    switch (purpose) {
    case 0x00: /* Purpose::CORE_P2P */
    case 0x01: /* Purpose::PLATFORM_P2P */
    case 0x02: /* Purpose::PLATFORM_HTTP */
        return true;
    default:
        return false;
    }
}

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

namespace {
inline constexpr uint8_t GetBIP155FromService(const CService& service)
{
    if (service.IsIPv4())  return 0x01 /* BIP155Network::IPV4 */;
    if (service.IsIPv6())  return 0x02 /* BIP155Network::IPV6 */;
    if (service.IsTor())   return 0x04 /* BIP155Network::TORV3 */;
    if (service.IsI2P())   return 0x05 /* BIP155Network::I2P */;
    if (service.IsCJDNS()) return 0x06 /* BIP155Network::CJDNS */;
    return 0xFF; /* invalid type */
}

inline constexpr bool IsTypeBIP155(const uint8_t& type)
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

class NetInfoEntry
{
private:
    static constexpr uint8_t INVALID_TYPE{0xFF};

    uint8_t type{INVALID_TYPE};
    CService data;

    // Used to directly read/write into members, needed to fake NetInfoEntry list in MnNetInfo
    friend class MnNetInfo;

public:
    NetInfoEntry() = default;
    ~NetInfoEntry() = default;

    NetInfoEntry(const CService& service) : type{GetBIP155FromService(service)}, data{service} {}

    bool operator<(const NetInfoEntry& rhs) const { return std::tie(type, data) < std::tie(rhs.type, rhs.data); }
    bool operator==(const NetInfoEntry& rhs) const { return std::tie(type, data) == std::tie(rhs.type, rhs.data); }
    bool operator!=(const NetInfoEntry& rhs) const { return !(*this == rhs); }

    template<typename Stream>
    void Serialize(Stream &s) const
    {
        s << type;
        if (IsTypeBIP155(type)) {
            s << data;
        } else {
            // Invalid type, bail out
            return;
        }
    }

    template<typename Stream>
    void Unserialize(Stream &s)
    {
        Clear();
        s >> type;
        if (IsTypeBIP155(type)) {
            s >> data;
        } else {
            // Invalid type, bail out
            return;
        }
    }

    void Clear()
    {
        type = INVALID_TYPE;
        data = CService();
    }

    const uint8_t& GetType() const { return type; }
    std::optional<std::reference_wrapper<const CService>> GetAddrPort() const;
    bool IsTriviallyValid() const;
    std::string ToString() const;
    std::string ToStringAddrPort() const;
};

using NetInfoList = std::vector<std::reference_wrapper<const NetInfoEntry>>;

class NetInfoInterface
{
public:
    virtual ~NetInfoInterface() = default;

    virtual NetInfoStatus AddEntry(const Purpose purpose, const std::string& input) = 0;
    virtual NetInfoList GetEntries() const = 0;

    virtual const CService& GetPrimary() const = 0;
    virtual bool IsEmpty() const = 0;
    virtual bool CanStorePlatform() const = 0;
    virtual NetInfoStatus Validate() const = 0;
    virtual UniValue ToJson() const = 0;
    virtual std::string ToString() const = 0;

    virtual void Clear() = 0;
};

class MnNetInfo final : public NetInfoInterface
{
private:
    // We still load/store a CService but we use a NetInfoEntry to help up avoid additional copies incurred
    // by constructing NetInfoEntry in-place for GetEntries()
    NetInfoEntry addr;

private:
    static NetInfoStatus ValidateService(const CService& service);

public:
    MnNetInfo() = default;
    ~MnNetInfo() = default;

    bool operator==(const MnNetInfo& rhs) const { return addr == rhs.addr; }
    bool operator!=(const MnNetInfo& rhs) const { return !(*this == rhs); }

    template<typename Stream>
    void Serialize(Stream &s) const
    {
        // We can safely discard NetInfoEntry::type as we can recalculate it on read
        s << addr.data;
    }

    template<typename Stream>
    void Unserialize(Stream &s)
    {
        Clear();
        s >> addr.data;
        // Refetch type to ensure NetInfoEntry::IsTriviallyValid() still passes
        addr.type = GetBIP155FromService(addr.data);
    }

    template <typename Stream>
    MnNetInfo(deserialize_type, Stream& s)
    {
        s >> *this;
    }

    NetInfoStatus AddEntry(const Purpose purpose, const std::string& input) override;
    NetInfoList GetEntries() const override;

    const CService& GetPrimary() const override { return addr.data; }
    bool IsEmpty() const override { return *this == MnNetInfo(); }
    bool CanStorePlatform() const override { return false; }
    NetInfoStatus Validate() const override { return ValidateService(addr.data); }
    UniValue ToJson() const override;
    std::string ToString() const override;

    void Clear() override { addr.Clear(); }
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

class ExtNetInfo final : public NetInfoInterface
{
private:
    static constexpr uint8_t NETINFO_FORMAT_VERSION{1};

    uint8_t m_version{NETINFO_FORMAT_VERSION};
    std::map<Purpose, std::set<NetInfoEntry>> m_data{};

private:
    // Type expected of first (i.e. "primary") entry.
    static constexpr uint8_t PRIMARY_ADDR_TYPE{/*BIP155Network::IPV4=*/0x01};

    NetInfoStatus ProcessCandidate(const Purpose& purpose, const NetInfoEntry& candidate);
    static NetInfoStatus ValidateService(const CService& service);

public:
    ExtNetInfo() = default;
    ~ExtNetInfo() = default;

    bool operator==(const ExtNetInfo& rhs) const { return m_version == rhs.m_version && m_data == rhs.m_data; }
    bool operator!=(const ExtNetInfo& rhs) const { return !(*this == rhs); }

    SERIALIZE_METHODS(ExtNetInfo, obj)
    {
        READWRITE(obj.m_version);
        if (obj.m_version == 0 || obj.m_version > NETINFO_FORMAT_VERSION) {
            return; // Don't bother with unknown versions
        }
        READWRITE(obj.m_data);
    }

    template <typename Stream>
    ExtNetInfo(deserialize_type, Stream& s)
    {
        s >> *this;
    }

    NetInfoStatus AddEntry(const Purpose purpose, const std::string& input) override;
    NetInfoList GetEntries() const override;

    const CService& GetPrimary() const override;
    bool IsEmpty() const override { return *this == ExtNetInfo(); }
    bool CanStorePlatform() const override { return true; }
    NetInfoStatus Validate() const override;
    UniValue ToJson() const override;
    std::string ToString() const override;

    void Clear() override
    {
        m_version = NETINFO_FORMAT_VERSION;
        m_data.clear();
    }
};

/* Selects NetInfoInterface implementation to use based on object version */
template <typename T1>
std::shared_ptr<NetInfoInterface> MakeNetInfo(const T1& obj)
{
    assert(obj.nVersion > 0);
    return std::make_shared<MnNetInfo>();
}

#endif // BITCOIN_EVO_NETINFO_H

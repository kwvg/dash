// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_NETINFO_H
#define BITCOIN_EVO_NETINFO_H

#include <netaddress.h>
#include <serialize.h>

#include <variant>

class CService;

static constexpr uint8_t EXTNETINFO_ENTRIES_LIMIT{32};
static constexpr uint8_t EXTNETINFO_FORMAT_VERSION{1};
static constexpr uint8_t EXTNETINFO_PRIMARY_ADDR_TYPE{/*BIP155Network::IPV4=*/0x01};

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
        return "invalid address";
    case NetInfoStatus::BadPort:
        return "invalid port";
    case NetInfoStatus::Malformed:
        return "malformed";
    case NetInfoStatus::Success:
        return "success";
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

namespace Purpose {
enum : uint8_t
{
    // Mandatory for masternodes
    CORE_P2P = 0,
    // Mandatory for EvoNodes
    PLATFORM_P2P = 1,
    // Optional for EvoNodes
    PLATFORM_HTTP = 2,
};
} // namespace Purpose

inline constexpr bool IsValidPurpose(const uint8_t purpose)
{
    switch (purpose) {
    case Purpose::CORE_P2P:
    case Purpose::PLATFORM_P2P:
    case Purpose::PLATFORM_HTTP:
        return true;
    }
    return false;
}

constexpr std::string PurposeToString(const uint8_t purpose) {
    switch (purpose) {
    case Purpose::CORE_P2P:
        return "CORE_P2P";
    case Purpose::PLATFORM_P2P:
        return "PLATFORM_P2P";
    case Purpose::PLATFORM_HTTP:
        return "PLATFORM_HTTP";
    }
    return "";
}

namespace Extensions {
enum : uint8_t
{
    DOMAINS = 0xD0,
};
} // namespace Purpose

class DomainPort
{
public:
    enum class Status : uint8_t
    {
        BadChar,
        BadCharPos,
        BadDotless,
        BadLabelCharPos,
        BadLabelLen,
        BadLen,
        BadPort,
        Malformed,

        Success
    };

    static constexpr std::string_view StatusToString(const DomainPort::Status code) {
        switch (code) {
        case DomainPort::Status::BadChar:
            return "invalid character";
        case DomainPort::Status::BadCharPos:
            return "bad domain character position";
        case DomainPort::Status::BadDotless:
            return "prohibited dotless";
        case DomainPort::Status::BadLabelCharPos:
            return "bad label character position";
        case DomainPort::Status::BadLabelLen:
            return "bad label length";
        case DomainPort::Status::BadLen:
            return "bad domain length";
        case DomainPort::Status::BadPort:
            return "bad port";
        case DomainPort::Status::Malformed:
            return "malformed";
        case DomainPort::Status::Success:
            return "success";
        } // no default case, so the compiler can warn about missing cases
        assert(false);
    }

private:
    std::string m_addr{};
    uint16_t m_port{0};

private:
    static DomainPort::Status ValidateDomain(const std::string& input);

public:
    DomainPort() = default;
    ~DomainPort() = default;

    bool operator<(const DomainPort& rhs) const { return std::tie(m_addr, m_port) < std::tie(rhs.m_addr, rhs.m_port); }
    bool operator==(const DomainPort& rhs) const { return std::tie(m_addr, m_port) == std::tie(rhs.m_addr, rhs.m_port); }
    bool operator!=(const DomainPort& rhs) const { return !(*this == rhs); }

    SERIALIZE_METHODS(DomainPort, obj)
    {
        READWRITE(obj.m_addr);
        READWRITE(obj.m_port);
    }

    DomainPort::Status Set(const std::string& addr, const uint16_t port);
    DomainPort::Status Validate() const;
    uint16_t GetPort() const { return m_port; }
    std::string ToStringAddr() const { return m_addr; }
    std::string ToStringAddrPort() const { return strprintf("%s:%d", m_addr, m_port); }
};

namespace {
inline constexpr uint8_t GetSupportedServiceType(const CService& service)
{
    if (service.IsIPv4()) {
        return 0x01; /* BIP155Network::IPV4 */
    } else if (service.IsIPv6() && !service.IsCJDNS()) {
        return 0x02; /* BIP155Network::IPV6 */
    } else if (service.IsTor()) {
        return 0x04; /* BIP155Network::TORV3 */
    } else if (service.IsI2P()) {
        return 0x05; /* BIP155Network::I2P */
    }
    return 0xFF; /* invalid type */
}

inline constexpr bool IsSupportedServiceType(const uint8_t& type)
{
    switch (type) {
    case 0x01: /* BIP155Network::IPV4 */
    case 0x02: /* BIP155Network::IPV6 */
    case 0x04: /* BIP155Network::TORV3 */
    case 0x05: /* BIP155Network::I2P */
        return true;
    default:
        return false;
    }
}

inline constexpr bool IsTypeExtension(const uint8_t& type)
{
    switch (type) {
    case Extensions::DOMAINS:
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

    uint8_t m_type{INVALID_TYPE};
    std::variant<std::monostate, CService, DomainPort> m_data{std::monostate{}};

    friend class MnNetInfo;

public:
    NetInfoEntry() = default;
    NetInfoEntry(const DomainPort& service) : m_type{Extensions::DOMAINS}, m_data{service} {}
    NetInfoEntry(const CService& service) : m_type{GetSupportedServiceType(service)}, m_data{service} {}
    template <typename Stream> NetInfoEntry(deserialize_type, Stream& s) { s >> *this; }

    ~NetInfoEntry() = default;

    bool operator<(const NetInfoEntry& rhs) const;
    bool operator==(const NetInfoEntry& rhs) const;
    bool operator!=(const NetInfoEntry& rhs) const { return !(*this == rhs); }

    template<typename Stream>
    void Serialize(Stream &s) const
    {
        s << m_type;
        if (const auto* data_ptr{std::get_if<CService>(&m_data)}; data_ptr != nullptr && IsSupportedServiceType(m_type)) {
            s << *data_ptr;
        } else if (const auto* data_ptr{std::get_if<DomainPort>(&m_data)}; data_ptr != nullptr && IsTypeExtension(m_type)) {
            s << *data_ptr;
        } else {
            // Invalid type, bail out
            return;
        }
    }

    void Serialize(CSizeComputer& s) const
    {
        auto size = ::GetSerializeSize(uint8_t{}, s.GetVersion());
        if (IsSupportedServiceType(m_type)) {
            size += ::GetSerializeSize(CService{}, s.GetVersion());
        } else if (IsTypeExtension(m_type)) {
            size += ::GetSerializeSize(DomainPort{}, s.GetVersion());
        }
        s.seek(size);
    }

    template<typename Stream>
    void Unserialize(Stream &s)
    {
        Clear();
        s >> m_type;
        if (IsSupportedServiceType(m_type)) {
            CService obj;
            s >> obj;
            m_data = obj;
        } else if (IsTypeExtension(m_type)) {
            DomainPort obj;
            s >> obj;
            m_data = obj;
        } else {
            // Invalid type, bail out
            return;
        }
    }

    void Clear()
    {
        m_type = INVALID_TYPE;
        m_data = std::monostate{};
    }

    std::optional<std::reference_wrapper<const CService>> GetAddrPort() const;
    std::optional<std::reference_wrapper<const DomainPort>> GetDomainPort() const;
    const uint8_t& GetType() const { return m_type; }
    bool IsTriviallyValid() const;
    std::string ToString() const;
    std::string ToStringAddrPort() const;
};

using NetInfoList = std::vector<std::reference_wrapper<const NetInfoEntry>>;

class NetInfoInterface
{
public:
    virtual ~NetInfoInterface() = default;

    virtual NetInfoStatus AddEntry(const uint8_t purpose, const std::string& service) = 0;
    virtual NetInfoList GetEntries() const = 0;

    virtual const CService& GetPrimary() const = 0;
    virtual bool HasEntries(uint8_t purpose) const = 0;
    virtual bool IsEmpty() const = 0;
    virtual NetInfoStatus Validate() const = 0;
    virtual std::string ToString() const = 0;

    virtual void Clear() = 0;
};

class MnNetInfo final : public NetInfoInterface
{
private:
    NetInfoEntry m_addr{};

private:
    static NetInfoStatus ValidateService(const CService& service);

public:
    MnNetInfo() = default;
    template <typename Stream> MnNetInfo(deserialize_type, Stream& s) { s >> *this; }

    ~MnNetInfo() = default;

    bool operator==(const MnNetInfo& rhs) const { return m_addr == rhs.m_addr; }
    bool operator!=(const MnNetInfo& rhs) const { return !(*this == rhs); }

    template<typename Stream>
    void Serialize(Stream &s) const
    {
        if (const auto* data_ptr{std::get_if<CService>(&m_addr.m_data)}; data_ptr != nullptr && IsSupportedServiceType(m_addr.m_type)) {
            s << *data_ptr;
        } else {
            s << CService();
        }
    }

    void Serialize(CSizeComputer& s) const
    {
        s.seek(::GetSerializeSize(CService{}, s.GetVersion()));
    }

    template<typename Stream>
    void Unserialize(Stream &s)
    {
        Clear();
        CService service;
        s >> service;
        m_addr = NetInfoEntry(service);
    }

    NetInfoStatus AddEntry(const uint8_t purpose, const std::string& service) override;
    NetInfoList GetEntries() const override;

    const CService& GetPrimary() const override;
    bool HasEntries(uint8_t purpose) const override { return purpose == Purpose::CORE_P2P && !IsEmpty(); }
    bool IsEmpty() const override { return *this == MnNetInfo(); }
    NetInfoStatus Validate() const override;
    std::string ToString() const override;

    void Clear() override { m_addr.Clear(); }
};

class ExtNetInfo final : public NetInfoInterface
{
private:
    uint8_t m_version{EXTNETINFO_FORMAT_VERSION};
    std::map<uint8_t, std::vector<NetInfoEntry>> m_data{};

private:
    NetInfoStatus ProcessCandidate(const uint8_t purpose, const NetInfoEntry& candidate);
    static NetInfoStatus ValidateService(const CService& service);
    static NetInfoStatus ValidateDomainPort(const DomainPort& service);

public:
    ExtNetInfo() = default;
    template <typename Stream> ExtNetInfo(deserialize_type, Stream& s) { s >> *this; }

    ~ExtNetInfo() = default;

    bool operator==(const ExtNetInfo& rhs) const { return m_version == rhs.m_version && m_data == rhs.m_data; }
    bool operator!=(const ExtNetInfo& rhs) const { return !(*this == rhs); }

    SERIALIZE_METHODS(ExtNetInfo, obj)
    {
        READWRITE(obj.m_version);
        if (obj.m_version == 0 || obj.m_version > EXTNETINFO_FORMAT_VERSION) {
            return; // Don't bother with unknown versions
        }
        READWRITE(obj.m_data);
    }

    NetInfoStatus AddEntry(const uint8_t purpose, const std::string& input) override;
    NetInfoList GetEntries() const override;

    const CService& GetPrimary() const override;
    bool HasEntries(uint8_t purpose) const override;
    bool IsEmpty() const override { return *this == ExtNetInfo(); }
    NetInfoStatus Validate() const override;
    std::string ToString() const override;

    void Clear() override
    {
        m_version = EXTNETINFO_FORMAT_VERSION;
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

class NetInfoSerWrapper
{
private:
    std::shared_ptr<NetInfoInterface>& m_data;

public:
    NetInfoSerWrapper() = delete;
    NetInfoSerWrapper(const NetInfoSerWrapper&) = delete;
    NetInfoSerWrapper(std::shared_ptr<NetInfoInterface>& data) : m_data{data} {}
    template <typename Stream> NetInfoSerWrapper(deserialize_type, Stream& s) { s >> *this; }

    ~NetInfoSerWrapper() = default;

    template<typename Stream>
    void Serialize(Stream &s) const
    {
        if (const auto& ptr{std::dynamic_pointer_cast<MnNetInfo>(m_data)}; ptr) {
            s << ptr;
        } else {
            throw std::ios_base::failure("Improperly constructed NetInfoInterface");
        }
    }

    void Serialize(CSizeComputer& s) const
    {
        s.seek(::GetSerializeSize(MnNetInfo{}, s.GetVersion()));
    }

    template<typename Stream>
    void Unserialize(Stream &s)
    {
        m_data.reset();
        std::shared_ptr<MnNetInfo> ptr;
        s >> ptr;
        m_data = std::move(ptr);
    }
};

#endif // BITCOIN_EVO_NETINFO_H

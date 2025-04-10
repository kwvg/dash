// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_NETINFO_H
#define BITCOIN_EVO_NETINFO_H

#include <netaddress.h>
#include <serialize.h>

#include <variant>

class CService;

enum NetInfoStatus : uint8_t
{
    BadInput,
    BadPort,
    Malformed,
    Success
};

constexpr std::string_view NISToString(const NetInfoStatus code) {
    switch (code) {
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

namespace {
inline constexpr uint8_t GetSupportedServiceType(const CService& service)
{
    if (service.IsIPv4()) {
        return 0x01; /* BIP155Network::IPV4 */
    }
    return 0xFF; /* invalid type */
}

inline constexpr bool IsSupportedServiceType(const uint8_t& type)
{
    switch (type) {
    case 0x01: /* BIP155Network::IPV4 */
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
    std::variant<std::monostate, CService> m_data{std::monostate{}};

    friend class MnNetInfo;

public:
    NetInfoEntry() = default;
    NetInfoEntry(const CService& service) : m_type{GetSupportedServiceType(service)}, m_data{service} {}

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
    bool IsTriviallyValid() const;
    std::string ToString() const;
    std::string ToStringAddrPort() const;
};

using NetInfoList = std::vector<std::reference_wrapper<const NetInfoEntry>>;

class MnNetInfo
{
private:
    NetInfoEntry m_addr{};

private:
    static NetInfoStatus ValidateService(const CService& service);

public:
    MnNetInfo() = default;
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

    NetInfoStatus AddEntry(const std::string& service);
    NetInfoList GetEntries() const;

    const CService& GetPrimary() const;
    bool IsEmpty() const { return *this == MnNetInfo(); }
    NetInfoStatus Validate() const;
    std::string ToString() const;

    void Clear() { m_addr.Clear(); }
};

#endif // BITCOIN_EVO_NETINFO_H

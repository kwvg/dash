// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_NETINFO_H
#define BITCOIN_EVO_NETINFO_H

#include <netaddress.h>
#include <serialize.h>

#include <variant>

class CService;

enum class NetInfoStatus : uint8_t {
    // Managing entries
    BadInput,

    // Validation
    BadAddress,
    BadPort,
    BadType,
    NotRoutable,

    Success
};

constexpr std::string_view NISToString(const NetInfoStatus code)
{
    switch (code) {
    case NetInfoStatus::BadAddress:
        return "invalid address";
    case NetInfoStatus::BadInput:
        return "invalid input";
    case NetInfoStatus::BadPort:
        return "invalid port";
    case NetInfoStatus::BadType:
        return "invalid address type";
    case NetInfoStatus::NotRoutable:
        return "unroutable address";
    case NetInfoStatus::Success:
        return "success";
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

class NetInfoEntry
{
private:
    enum NetInfoType : uint8_t {
        Service = 0x01,
        Invalid = 0xff
    };

private:
    uint8_t m_type{NetInfoType::Invalid};
    std::variant<std::monostate, CService> m_data{std::monostate{}};

public:
    NetInfoEntry() = default;
    NetInfoEntry(const CService& service)
    {
        if (!service.IsValid()) return;
        m_type = NetInfoType::Service;
        m_data = service;
    }

    ~NetInfoEntry() = default;

    bool operator<(const NetInfoEntry& rhs) const;
    bool operator==(const NetInfoEntry& rhs) const;
    bool operator!=(const NetInfoEntry& rhs) const { return !(*this == rhs); }

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        switch (m_type) {
        case NetInfoType::Service: {
            if (const auto* data_ptr{std::get_if<CService>(&m_data)}; data_ptr != nullptr) {
                s << m_type;
                s << *data_ptr;
            } else {
                // Flagged as storing CService but no CService found, write object as invalid
                s << NetInfoType::Invalid;
            }
            break;
        }
        default: {
            // Invalid type, write object as invalid
            s << NetInfoType::Invalid;
            break;
        }
        };
    }

    void Serialize(CSizeComputer& s) const
    {
        auto size = ::GetSerializeSize(uint8_t{}, s.GetVersion());
        if (m_type == NetInfoType::Service) {
            size += ::GetSerializeSize(CService{}, s.GetVersion());
        }
        s.seek(size);
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        Clear();

        s >> m_type;
        switch (m_type) {
        case NetInfoType::Service: {
            try {
                CService obj;
                s >> obj;
                m_data = obj;
            } catch (const std::ios_base::failure&) {
                // Flagged as storing CService but no CService found, reset to mark object as invalid
                Clear();
            }
            break;
        }
        default: {
            // Invalid type, reset to mark object as invalid
            Clear();
            break;
        }
        };
    }

    void Clear()
    {
        m_type = NetInfoType::Invalid;
        m_data = std::monostate{};
    }

    std::optional<std::reference_wrapper<const CService>> GetAddrPort() const;
    bool IsTriviallyValid() const;
    std::string ToString() const;
    std::string ToStringAddrPort() const;
};

using CServiceList = std::vector<std::reference_wrapper<const CService>>;

class MnNetInfo
{
private:
    CService m_addr{};

private:
    static NetInfoStatus ValidateService(const CService& service);

public:
    MnNetInfo() = default;
    ~MnNetInfo() = default;

    bool operator==(const MnNetInfo& rhs) const { return m_addr == rhs.m_addr; }
    bool operator!=(const MnNetInfo& rhs) const { return !(*this == rhs); }

    SERIALIZE_METHODS(MnNetInfo, obj)
    {
        READWRITE(obj.m_addr);
    }

    NetInfoStatus AddEntry(const std::string& service);
    CServiceList GetEntries() const;

    const CService& GetPrimary() const { return m_addr; }
    bool IsEmpty() const { return *this == MnNetInfo(); }
    NetInfoStatus Validate() const { return ValidateService(m_addr); }
    std::string ToString() const;

    void Clear() { m_addr = CService(); }
};

#endif // BITCOIN_EVO_NETINFO_H

// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_NETINFO_H
#define BITCOIN_EVO_NETINFO_H

#include <netaddress.h>
#include <serialize.h>
#include <streams.h>

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
    Malformed,

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
    case NetInfoStatus::Malformed:
        return "malformed";
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
    void Serialize(Stream& s_) const
    {
        OverrideStream<Stream> s(&s_, s_.GetType(), s_.GetVersion() | ADDRV2_FORMAT);

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
            size += ::GetSerializeSize(CService{}, s.GetVersion() | ADDRV2_FORMAT);
        }
        s.seek(size);
    }

    template <typename Stream>
    void Unserialize(Stream& s_)
    {
        Clear();

        OverrideStream<Stream> s(&s_, s_.GetType(), s_.GetVersion() | ADDRV2_FORMAT);

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

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        if (const auto& service{m_addr.GetAddrPort()}; service.has_value()) {
            s << service->get();
        } else {
            s << CService{};
        }
    }

    void Serialize(CSizeComputer& s) const
    {
        s.seek(::GetSerializeSize(CService{}, s.GetVersion()));
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        CService service;
        s >> service;
        m_addr = NetInfoEntry{service};
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

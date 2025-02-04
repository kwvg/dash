// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_EXTENDED_H
#define BITCOIN_EVO_EXTENDED_H

#include <evo/common.h>
#include <netaddress.h>
#include <serialize.h>

#include <optional>
#include <string>
#include <variant>

class CDeterministicMNManager;
class CService;

static constexpr uint8_t MNADDR_ENTRIES_LIMIT{32};

// All extensions should start with 0xDn where n is your extension number to avoid
// conflicts with BIP155 network IDs.
enum class Extensions : uint8_t
{
    // Extension A in Appendix C of DIP3. It rolls off the tongue easily, promise.
    DOMAINS = 0xD0
};
template<> struct is_serializable_enum<Extensions> : std::true_type {};

using DomainPort = std::pair<std::string, uint16_t>;

class MnNetInfo
{
private:
    struct NetInfo
    {
    private:
        using NetAddrVariant = std::pair<CNetAddr::BIP155Network, CNetAddr>;
        using StrAddrVariant = std::pair<Extensions, std::string>;

        // Type of address, could be BIP155 type or an extension as defined in
        // Appendix C DIP3 extension. Serialized as uint8_t.
        std::variant<std::monostate, NetAddrVariant, StrAddrVariant> type_addr;
        uint16_t port;

    private:
        friend class MnNetInfo;

        // Used in Validate()
        static std::optional<std::string> ValidateNetAddr(const NetAddrVariant& input, const uint16_t& port);
        // Used in Validate()
        static std::optional<std::string> ValidateStrAddr(const StrAddrVariant& input, const uint16_t& port);

        // Used in RemoveEntry()
        std::optional<CService> GetCService() const;
        // Used in RemoveEntry()
        std::optional<DomainPort> GetDomainPort() const;

    public:
        NetInfo() = default; // should be delete but deserialization code becomes very angry if we do
        ~NetInfo() = default;

        NetInfo(CNetAddr::BIP155Network type, CService service) : type_addr{std::make_pair(type, service)}, port{service.GetPort()} {}
        NetInfo(CNetAddr::BIP155Network type, CNetAddr netaddr, uint16_t port) : type_addr{std::make_pair(type, netaddr)}, port{port} {}
        NetInfo(Extensions type, std::string straddr, uint16_t port) : type_addr{std::make_pair(type, straddr)}, port{port} {}

        bool operator==(const NetInfo& rhs);

        template<typename Stream>
        void Serialize(Stream &s) const
        {
            s << type_addr.index();
            switch(type_addr.index()) {
            case 1 /* NetAddrVariant */: {
                s << std::get<NetAddrVariant>(type_addr);
                break;
            }
            case 2 /* StrAddrVariant */: {
                s << std::get<StrAddrVariant>(type_addr);
                break;
            }
            default /* std::monostate or something unexpected */: return;
            }
            s << port;
        }

        template<typename Stream>
        void Unserialize(Stream &s)
        {
            Clear();
            uint8_t type_idx;
            s >> type_idx;
            switch (type_idx) {
            case 1 /* NetAddrVariant */: {
                NetAddrVariant net_addr;
                s >> net_addr;
                type_addr = net_addr;
                break;
            }
            case 2 /* StrAddrVariant */: {
                StrAddrVariant str_addr;
                s >> str_addr;
                type_addr = str_addr;
                break;
            }
            default /* std::monostate or something unexpected */: return;
            }
           s << port;
        }

        void Clear()
        {
            type_addr = std::monostate{};
            port = 0;
        }

        // Dispatch function to Validate{Net,Str}Addr()
        std::optional<std::string> Validate();
    };

private:
    static constexpr uint8_t NETINFO_FORMAT_VERSION{1};

    // The format corresponds to the on-disk format *and* validation rules. Any changes
    // to MnNetInfo, NetInfo, Purpose or Extensions will require incrementing this value.
    uint8_t version{NETINFO_FORMAT_VERSION};
    std::map<Purpose, std::vector<NetInfo>> data{};

    // Used by AddEntry()
    std::vector<NetInfo>& GetOrAddEntries(Purpose purpose);

public:
    MnNetInfo() = default;
    ~MnNetInfo() = default;

    // Add or remove entry from set of entries
    std::optional<std::string> AddEntry(Purpose purpose, CService service);
    std::optional<std::string> AddEntry(Purpose purpose, std::string addr, uint16_t port);
    std::optional<std::string> RemoveEntry(CService service);
    std::optional<std::string> RemoveEntry(DomainPort addr);

    const std::optional<std::vector<CService>> GetAddrPorts(Purpose purpose) const;
    const std::optional<std::vector<DomainPort>> GetDomainPorts(Purpose purpose) const;

    SERIALIZE_METHODS(MnNetInfo, obj)
    {
        READWRITE(obj.version);
        READWRITE(obj.data);
    }
};

#endif // BITCOIN_EVO_EXTENDED_H

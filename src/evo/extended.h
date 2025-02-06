// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_EXTENDED_H
#define BITCOIN_EVO_EXTENDED_H

#include <evo/common.h>
#include <netaddress.h>
#include <serialize.h>
#include <util/underlying.h>

#include <optional>
#include <string>
#include <variant>

class CDeterministicMNManager;
class CService;

class UniValue;

static constexpr uint8_t MNADDR_ENTRIES_LIMIT{32};

// All extensions should start with 0xDn where n is your extension number to avoid
// conflicts with BIP155 network IDs.
enum class Extensions : uint8_t
{
    // Extension A in Appendix C of DIP3. It rolls off the tongue easily, promise.
    DOMAINS = 0xD0
};
template<> struct is_serializable_enum<Extensions> : std::true_type {};

class MnNetInfo
{
private:
    struct NetInfo
    {
    private:
        using AddrVariant = std::variant<std::monostate, CNetAddr, std::string>;

        // Type of address, could be BIP155 type or an extension as defined in
        // Appendix C DIP3 extension. Serialized as uint8_t.
        uint8_t type;
        AddrVariant addr;
        uint16_t port;

    private:
        friend class MnNetInfo;

        // Used in Validate()
        static MnNetStatus ValidateNetAddr(const uint8_t& type, const CNetAddr& input, const uint16_t& port);
        // Used in Validate()
        static MnNetStatus ValidateStrAddr(const uint8_t& type, const std::string& input, const uint16_t& port);

        // Used in RemoveEntry()
        std::optional<CService> GetCService() const;
        // Used in RemoveEntry()
        std::optional<DomainPort> GetDomainPort() const;

    public:
        NetInfo() = default; // should be delete but deserialization code becomes very angry if we do
        ~NetInfo() = default;

        NetInfo(CNetAddr::BIP155Network type, CService service) : type{type}, addr{service}, port{service.GetPort()} {}
        NetInfo(CNetAddr::BIP155Network type, CNetAddr netaddr, uint16_t port) : type{type}, addr{netaddr}, port{port} {}
        NetInfo(Extensions type, std::string straddr, uint16_t port) : type{ToUnderlying(type)}, addr{straddr}, port{port} {}

        bool operator==(const NetInfo& rhs);

        template<typename Stream>
        void Serialize(Stream &s) const
        {
            s << type;
            switch (type) {
            case 0x01: /* BIP155Network::IPV4 */
            case 0x02: /* BIP155Network::IPV6 */
            case 0x03: /* BIP155Network::TORV2 */
            case 0x04: /* BIP155Network::TORV3 */
            case 0x05: /* BIP155Network::I2P */
            case 0x06: /* BIP155Network::CJDNS */
            {
                s << std::get<CNetAddr>(addr);
                break;
            }
            case 0xD0: /* Extensions::DOMAINS */
            {
                s << std::get<std::string>(addr);
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
            s >> type;
            switch (type) {
            case 0x01: /* BIP155Network::IPV4 */
            case 0x02: /* BIP155Network::IPV6 */
            case 0x03: /* BIP155Network::TORV2 */
            case 0x04: /* BIP155Network::TORV3 */
            case 0x05: /* BIP155Network::I2P */
            case 0x06: /* BIP155Network::CJDNS */
            {
                CNetAddr obj;
                s >> obj;
                addr = obj;
                break;
            }
            case 0xD0: /* Extensions::DOMAINS */
            {
                std::string obj;
                s >> obj;
                addr = obj;
                break;
            }
            default /* std::monostate or something unexpected */: return;
            }
           s << port;
        }

        void Clear()
        {
            type = 0;
            addr = std::monostate{};
            port = 0;
        }

        // Dispatch function to Validate{Net,Str}Addr()
        MnNetStatus Validate();
        // For debug logs
        std::string ToString() const;
        // For RPCs
        std::string ToStringAddrPort() const;
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
    MnNetStatus AddEntry(Purpose purpose, CService service);
    MnNetStatus AddEntry(Purpose purpose, DomainPort service);
    MnNetStatus RemoveEntry(CService service);
    MnNetStatus RemoveEntry(DomainPort service);

    // Used in tests
    const std::vector<CService> GetAddrPorts(Purpose purpose) const;
    const std::vector<DomainPort> GetDomainPorts(Purpose purpose) const;

    SERIALIZE_METHODS(MnNetInfo, obj)
    {
        READWRITE(obj.version);
        READWRITE(obj.data);
    }

    UniValue ToJson() const;
    std::string ToString() const;
};

#endif // BITCOIN_EVO_EXTENDED_H

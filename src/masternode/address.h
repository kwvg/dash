// Copyright (c) 2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MASTERNODE_ADDRESS_H
#define BITCOIN_MASTERNODE_ADDRESS_H

#include <netaddress.h>
#include <serialize.h>
#include <uint256.h>

#include <optional>
#include <string>
#include <variant>

class CDeterministicMNManager;
class CService;

/* ---------------------- address format -------------------------------------------------------------------- */

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

/* ---------------------- transaction format ---------------------------------------------------------------- */

// TODO: Currently this corresponds to the index, is this a good idea?
enum class Purpose : uint8_t
{
    // Mandatory for all masternodes
    CORE_P2P = 0,
    // Mandatory for all EvoNodes
    PLATFORM_P2P = 1,
    // Optional for EvoNodes
    PLATFORM_API = 2
};
template<> struct is_serializable_enum<Purpose> : std::true_type {};

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
        std::optional<std::string> GetString() const;

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
    std::optional<std::string> RemoveEntry(std::string addr);

    SERIALIZE_METHODS(MnNetInfo, obj)
    {
        READWRITE(obj.version);
        READWRITE(obj.data);
    }
};

#endif // BITCOIN_MASTERNODE_ADDRESS_H

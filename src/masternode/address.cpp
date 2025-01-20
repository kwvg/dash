// Copyright (c) 2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/address.h>

#include <bech32.h>
#include <chainparams.h>
#include <evo/deterministicmns.h>
#include <netaddress.h>
#include <netbase.h>
#include <protocol.h>
#include <serialize.h>
#include <util/check.h>
#include <util/strencodings.h>

#include <vector>

namespace {
template <typename T1>
inline std::nullopt_t error(T1& lhs, T1 rhs) { lhs = rhs; return std::nullopt; }

std::string HashToAddress(const uint256 hash) {
    std::vector<uint8_t> data = {0};
    data.reserve(1 + (hash.size() * 8 + 3) / 5);
    ConvertBits</*frombits=*/8, /*tobits=*/5, /*pad=*/true>([&](unsigned char c) { data.push_back(c); }, hash.begin(), hash.end());
    return bech32::Encode(bech32::Encoding::BECH32M, Params().MnAddrHRP(), data);
}

std::optional<uint256> AddressToHash(const std::string addr, MnAddr::DecodeStatus& status) {
    uint256 hash;
    const auto dec = bech32::Decode(addr);
    if (dec.encoding != bech32::Encoding::BECH32M) {
        return error(status, MnAddr::DecodeStatus::NotBech32m);
    }
    if (dec.data.empty()) {
        return error(status, MnAddr::DecodeStatus::DataEmpty);
    }
    if (dec.data[0] != /*expected_version*/0) {
        return error(status, MnAddr::DecodeStatus::DataVersionBad);
    }
    if (dec.hrp != Params().MnAddrHRP()) {
        return error(status, MnAddr::DecodeStatus::HRPBad);
    }
    std::vector<uint8_t> data;
    data.reserve(((dec.data.size() - 1) * 5) / 8);
    if (!ConvertBits</*frombits=*/5, /*tobits=*/8, /*pad=*/false>([&](unsigned char c) { data.push_back(c); }, dec.data.begin() + 1, dec.data.end())) {
        return error(status, MnAddr::DecodeStatus::DataPaddingBad);
    }
    if (data.size() != hash.size()) {
        return error(status, MnAddr::DecodeStatus::DataSizeBad);
    }
    std::copy(data.begin(), data.end(), hash.begin());
    status = MnAddr::DecodeStatus::Success;
    return hash;
}
} // anonymous namespace

MnAddr::MnAddr(uint256 hash) :
    protx_hash{hash},
    is_valid{true},
    address{HashToAddress(protx_hash)}
{}

MnAddr::MnAddr(std::string addr, MnAddr::DecodeStatus& status) :
    protx_hash{[&addr, &status]() {
        if (auto hash_opt = AddressToHash(addr, status); hash_opt.has_value()) { return hash_opt.value(); } else { return uint256::ZERO; }
    }()},
    is_valid{protx_hash != uint256::ZERO},
    address{[&addr, this]() { if (is_valid) { return addr; } else { return std::string{""}; } }()}
{}

MnAddr::~MnAddr() {}

std::string DSToString(MnAddr::DecodeStatus status)
{
    switch (status) {
        case MnAddr::DecodeStatus::NotBech32m:
            return "bad encoding";
        case MnAddr::DecodeStatus::HRPBad:
            return "unsupported prefix or incorrect network";
        case MnAddr::DecodeStatus::DataEmpty:
            return "no data encoded";
        case MnAddr::DecodeStatus::DataVersionBad:
            return "bad version";
        case MnAddr::DecodeStatus::DataPaddingBad:
            return "bad data padding";
        case MnAddr::DecodeStatus::DataSizeBad:
            return "unexpected data size";
        case MnAddr::DecodeStatus::Success:
            return "c: yay !!!";
    }  // no default case, so the compiler can warn about missing cases

    assert(false);
}

std::optional<CService> GetConnectionDetails(CDeterministicMNManager& dmnman, const MnAddr mn_addr, std::string& error_str)
{
    if (!mn_addr.IsValid()) {
        return error(error_str, strprintf("Invalid address"));
    }
    const auto mn_list = dmnman.GetListAtChainTip();
    auto mn = mn_list.GetMN(mn_addr.GetHash());
    if (!mn) {
        return error(error_str, strprintf("Masternode not found in list"));
    }
    return Assert(mn->pdmnState)->addr;
}

// This doesn't do anything for now but should help us evaluate the kind of interface needed so we can
// abstract currently deployed logic to resemble an interface that closely matches this one to make the
// transition smoother.
namespace {
std::optional<CNetAddr::BIP155Network> GetBIP155Service(CNetAddr addr)
{
    if (addr.IsCJDNS()) return CNetAddr::BIP155Network::CJDNS;
    else if (addr.IsTor()) return CNetAddr::BIP155Network::TORV3;
    else if (addr.IsI2P()) return CNetAddr::BIP155Network::I2P;
    else if (addr.IsIPv4()) return CNetAddr::BIP155Network::IPV4;
    else if (addr.IsIPv6()) return CNetAddr::BIP155Network::IPV6;
    else return std::nullopt;
}

bool HasBadTLD(const std::string& str) {
    const std::vector<std::string_view> blocklist{
        ".local",
        ".intranet",
        ".internal",
        ".private",
        ".corp",
        ".home",
        ".lan",
        ".home.arpa"
    };
    for (const auto& tld : blocklist) {
        if (tld.size() > str.size()) continue;
        if (std::equal(tld.rbegin(), tld.rend(), str.rbegin())) return true;
    }
    return false;
}
} // anonymous namespace

static constexpr std::string_view SAFE_CHARS_RFC1035{"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-"};

template<> struct is_serializable_enum<CNetAddr::BIP155Network> : std::true_type {};

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

std::optional<CService> MnNetInfo::NetInfo::GetCService() const
{
    const auto* ptr{std::get_if<NetAddrVariant>(&type_addr)};
    if (ptr != nullptr) { return CService{ptr->second, port}; }
    return std::nullopt;
}

std::optional<std::string> MnNetInfo::NetInfo::GetString() const
{
    const auto* ptr{std::get_if<StrAddrVariant>(&type_addr)};
    if (ptr != nullptr) { return ptr->second; }
    return std::nullopt;
}

bool MnNetInfo::NetInfo::operator==(const NetInfo& rhs) {
    if (port != rhs.port) return false;
    return std::visit([](auto&& lhs, auto&& rhs) -> bool {
        if constexpr (std::is_same_v<std::decay_t<decltype(lhs)>, std::decay_t<decltype(rhs)>>) {
            return lhs == rhs;
        }
        return false;
    }, type_addr, rhs.type_addr);
}

std::optional<std::string> MnNetInfo::NetInfo::Validate()
{
    return std::visit([this](auto&& input) -> std::optional<std::string> {
        using T1 = std::decay_t<decltype(input)>;
        if constexpr (std::is_same_v<T1, NetAddrVariant>) {
            return ValidateNetAddr(input, port);
        } else if constexpr (std::is_same_v<T1, StrAddrVariant>) {
            return ValidateStrAddr(input, port);
        } else {
            return "empty object, nothing to validate";
        }
    }, type_addr);
}

std::optional<std::string> MnNetInfo::NetInfo::ValidateNetAddr(const NetAddrVariant& input, const uint16_t& port)
{
    const auto& [type, net_addr] = input;
    if (!net_addr.IsValid()) {
        return "invalid address";
    }
    if (IsBadPort(port)) {
        return "bad port";
    }
    if (net_addr.IsLocal()) {
        return "disallowed address, provided local address";
    }
    if (type == CNetAddr::BIP155Network::TORV2) {
        return "disallowed type, TorV2 deprecated";
    }
    return std::nullopt;
}

std::optional<std::string> MnNetInfo::NetInfo::ValidateStrAddr(const StrAddrVariant& input, const uint16_t& port)
{
    const auto& [type, str_addr] = input;
    if (IsBadPort(port) && port != 80 && port != 443) {
        return "bad port";
    }
    if (str_addr.length() > 253 || str_addr.length() < 4) {
        return "bad domain length";
    }
    bool is_dotted;
    for (char c : str_addr) {
        if (SAFE_CHARS_RFC1035.find(c) == std::string::npos) {
            return "prohibited domain character";
        }
    }
    if (str_addr.at(0) == '.' || str_addr.at(str_addr.length() - 1) == '.') {
        return "prohibited domain character position";
    }
    std::vector<std::string> labels{SplitString(str_addr, '.')};
    if (labels.size() < 2) {
        return "prohibited dotless";
    }
    if (HasBadTLD(str_addr)) {
        return "prohibited tld";
    }
    for (const auto& label : labels) {
        if (label.empty() || label.length() > 63) {
            return "bad label length";
        }
        if (label.at(0) == '-' or label.at(label.length() - 1) == '-') {
            return "prohibited label character position";
        }
    }
    return std::nullopt;
}

std::vector<MnNetInfo::NetInfo>& MnNetInfo::GetOrAddEntries(Purpose purpose) {
    for (auto& [_purpose, _entry] : data) {
        if (_purpose != purpose) continue;
        return _entry;
    }
    auto [it, status] = data.try_emplace(purpose, std::vector<NetInfo>{});
    assert(status); // We did just check to see if our value already existed, try_emplace
                    // shouldn't fail.
    return it->second;
}

std::optional<std::string> MnNetInfo::AddEntry(Purpose purpose, CService service)
{
    const auto opt_type = GetBIP155Service(service);
    if (!opt_type.has_value()) {
        return "disallowed address, cannot determine BIP155 type";
    }

    NetInfo candidate{opt_type.value(), service};
    if (auto err_str = candidate.Validate(); err_str.has_value()) {
        return err_str.value();
    }

    auto& entries{GetOrAddEntries(purpose)};
    if (std::find(entries.begin(), entries.end(), candidate) != entries.end()) {
        return "duplicate entry";
    }

    entries.push_back(candidate);
    return std::nullopt;
}

// TODO: Find a way to share code with CService overload, it's like 2/3rd overlapping
std::optional<std::string> MnNetInfo::AddEntry(Purpose purpose, std::string addr, uint16_t port)
{
    if (purpose != Purpose::PLATFORM_API) {
        return "domains allowed only for platform api";
    }

    NetInfo candidate{Extensions::DOMAINS, addr, port};
    if (auto err_str = candidate.Validate(); err_str.has_value()) {
        return err_str.value();
    }

    auto& entries{GetOrAddEntries(purpose)};
    // i want to use std::set so we can skip having to check for duplicates
    // but serialization code is unhappy if i do that
    if (std::find(entries.begin(), entries.end(), candidate) != entries.end()) {
        return "duplicate entry";
    }

    entries.push_back(candidate);
    return std::nullopt;
}

std::optional<std::string> MnNetInfo::RemoveEntry(CService service)
{
    for (auto& [purpose, entries] : data) {
        auto past_size{entries.size()};
        entries.erase(std::remove_if(entries.begin(), entries.end(), [&service](auto input) -> bool {
            auto _input{input.GetCService()};
            return _input.has_value() && _input.value() == service;
        }));
        // it's okay to not go through every set of purposes because they should all be unique anyway
        // TODO: enforce this
        if (entries.size() > past_size) return std::nullopt;
    }
    return "unable to find entry";
}

std::optional<std::string> MnNetInfo::RemoveEntry(std::string addr)
{
    for (auto& [purpose, entries] : data) {
        auto past_size{entries.size()};
        entries.erase(std::remove_if(entries.begin(), entries.end(), [&addr](auto input) -> bool {
            auto _input{input.GetString()}; // <--- only thing that differentiates it from RemoveEntry(CService)
            return _input.has_value() && _input.value() == addr;
        }));
        if (entries.size() > past_size) return std::nullopt;
    }
    return "unable to find entry";
}

// Stub function to ensure compiler verifies serialization functions actually work
void TestSerializationMagic()
{
    MnNetInfo network_info;
    CDataStream s(SER_NETWORK, PROTOCOL_VERSION);
    s << network_info;
    s >> network_info;
}

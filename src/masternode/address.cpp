// Copyright (c) 2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/address.h>

#include <bech32.h>
#include <chainparams.h>
#include <evo/deterministicmns.h>
#include <netaddress.h>
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

static constexpr uint8_t NETINFO_FORMAT_VERSION{1};

template<> struct is_serializable_enum<CNetAddr::BIP155Network> : std::true_type {};

enum class Purpose : uint8_t
{
    // Mandatory for all masternodes
    CORE_P2P = 0x01,
    // Mandatory for all EvoNodes
    PLATFORM_P2P = 0x02,
    // Optional for EvoNodes
    PLATFORM_API = 0x03,

    // Optional for all masternodes, available on devnet only. Does nothing for now.
    RESERVED_TESTING = 0x00
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
        using NetAddrVariant = std::pair<CNetAddr::BIP155Network, CNetAddr>;
        using StrAddrVariant = std::pair<Extensions, std::string>;

        // Type of address, could be BIP155 type or an extension as defined in
        // Appendix C DIP3 extension. Serialized as uint8_t.
        std::variant<std::monostate, NetAddrVariant, StrAddrVariant> type_addr;
        uint16_t port;

        NetInfo() = default;
        ~NetInfo() = default;

        template<typename Stream>
        void Serialize(Stream &s) const
        {
            s << type_addr.index();
            switch(type_addr.index()) {
            case 0 /* std::monotype */: return; // There's nothing more to serialize, bail out!
            case 1 /* NetAddrVariant */: {
                s << std::get<NetAddrVariant>(type_addr);
                break;
            }
            case 2 /* StrAddrVariant */: {
                s << std::get<StrAddrVariant>(type_addr);
                break;
            }
            default: {
                throw std::ios_base::failure("invalid variant");
            }
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
            case 0 /* std::monotype */: return; // Invalid data, nothing more to read!
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
            default: {
                throw std::ios_base::failure("invalid variation byte");
            }
            }
           s << port;
        }

        void Clear()
        {
            type_addr = std::monostate{};
            port = 0;
        }
    };

    struct NetEntry
    {
        Purpose purpose;
        std::vector<NetInfo> entries;

        NetEntry() = default;
        ~NetEntry() = default;

        SERIALIZE_METHODS(NetEntry, obj)
        {
            READWRITE(obj.purpose);
            READWRITE(obj.entries);
        }
    };

    // The format corresponds to the on-disk format *and* validation rules. Any changes
    // to MnNetInfo, NetEntry, NetInfo, Purpose or Extensions will require incrementing
    // this value.
    uint8_t version{NETINFO_FORMAT_VERSION};
    std::vector<NetEntry> data{};

public:
    MnNetInfo() = default;
    ~MnNetInfo() = default;

    SERIALIZE_METHODS(MnNetInfo, obj)
    {
        READWRITE(obj.version);
        READWRITE(obj.data);
    }
};

// Stub function to ensure compiler verifies serialization functions actually work
void TestSerializationMagic()
{
    MnNetInfo network_info;
    CDataStream s(SER_NETWORK, PROTOCOL_VERSION);
    s << network_info;
    s >> network_info;
}

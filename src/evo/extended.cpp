// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/extended.h>

#include <netbase.h>
#include <protocol.h>
#include <util/check.h>
#include <util/strencodings.h>
#include <version.h>

#include <vector>

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
bool IsAllowedPort(uint16_t port) {
    switch (port) {
    case 80:
    case 443:
        return true;
    }
    return false;
}
} // anonymous namespace

static constexpr std::string_view SAFE_CHARS_RFC1035{"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-"};

template<> struct is_serializable_enum<CNetAddr::BIP155Network> : std::true_type {};

std::optional<CService> MnNetInfo::NetInfo::GetCService() const
{
    const auto* ptr{std::get_if<CNetAddr>(&addr)};
    if (ptr != nullptr) { return CService{*ptr, port}; }
    return std::nullopt;
}

std::optional<DomainPort> MnNetInfo::NetInfo::GetDomainPort() const
{
    const auto* ptr{std::get_if<std::string>(&addr)};
    if (ptr != nullptr) { return std::make_pair(*ptr, port); }
    return std::nullopt;
}

bool MnNetInfo::NetInfo::operator==(const NetInfo& rhs) {
    if (port != rhs.port || type != rhs.type) return false;
    return std::visit([](auto&& lhs, auto&& rhs) -> bool {
        if constexpr (std::is_same_v<std::decay_t<decltype(lhs)>, std::decay_t<decltype(rhs)>>) {
            return lhs == rhs;
        }
        return false;
    }, addr, rhs.addr);
}

std::optional<std::string> MnNetInfo::NetInfo::Validate()
{
    return std::visit([this](auto&& input) -> std::optional<std::string> {
        using T1 = std::decay_t<decltype(input)>;
        if constexpr (std::is_same_v<T1, CNetAddr>) {
            return ValidateNetAddr(type, input, port);
        } else if constexpr (std::is_same_v<T1, std::string>) {
            return ValidateStrAddr(type, input, port);
        } else {
            return "empty object, nothing to validate";
        }
    }, addr);
}

std::optional<std::string> MnNetInfo::NetInfo::ValidateNetAddr(const uint8_t& type, const CNetAddr& input, const uint16_t& port)
{
    if (!input.IsValid()) {
        return "invalid address";
    }
    if (IsBadPort(port) || port == 0) {
        return "bad port";
    }
    if (input.IsLocal()) {
        return "disallowed address, provided local address";
    }
    if (type == CNetAddr::BIP155Network::TORV2) {
        return "disallowed type, TorV2 deprecated";
    }
    return std::nullopt;
}

std::optional<std::string> MnNetInfo::NetInfo::ValidateStrAddr(const uint8_t& type, const std::string& input, const uint16_t& port)
{
    if ((IsBadPort(port) && !IsAllowedPort(port)) || port == 0) {
        return "bad port";
    }
    if (input.length() > 253 || input.length() < 4) {
        return "bad domain length";
    }
    bool is_dotted;
    for (char c : input) {
        if (SAFE_CHARS_RFC1035.find(c) == std::string::npos) {
            return "prohibited domain character";
        }
    }
    if (input.at(0) == '.' || input.at(input.length() - 1) == '.') {
        return "prohibited domain character position";
    }
    std::vector<std::string> labels{SplitString(input, '.')};
    if (labels.size() < 2) {
        return "prohibited dotless";
    }
    if (HasBadTLD(input)) {
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

    if (entries.size() > MNADDR_ENTRIES_LIMIT) {
        return "too many entries";
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

    if (entries.size() > MNADDR_ENTRIES_LIMIT) {
        return "too many entries";
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

std::optional<std::string> MnNetInfo::RemoveEntry(DomainPort addr)
{
    for (auto& [purpose, entries] : data) {
        auto past_size{entries.size()};
        entries.erase(std::remove_if(entries.begin(), entries.end(), [&addr](auto input) -> bool {
            auto _input{input.GetDomainPort()}; // <--- only thing that differentiates it from RemoveEntry(CService)
            return _input.has_value() && _input.value() == addr;
        }));
        if (entries.size() > past_size) return std::nullopt;
    }
    return "unable to find entry";
}

const std::optional<std::vector<CService>> MnNetInfo::GetAddrPorts(Purpose purpose) const
{
    // TODO: less ugly way to do it?
    const std::vector<MnNetInfo::NetInfo>* entries_ptr{nullptr};
    for (auto& [_purpose, _entry] : data) {
        if (_purpose != purpose) continue;
        entries_ptr = &_entry; break;
    }
    if (entries_ptr == nullptr) return std::nullopt;

    std::vector<CService> ret{};
    for (const auto& entry : *entries_ptr) {
        if (auto service = entry.GetCService(); service.has_value()) {
            ret.push_back(service.value());
        }
    }
    if (ret.empty()) return std::nullopt;

    return ret;
}

const std::optional<std::vector<DomainPort>> MnNetInfo::GetDomainPorts(Purpose purpose) const
{
    // TODO: less ugly way to do it?
    const std::vector<MnNetInfo::NetInfo>* entries_ptr{nullptr};
    for (auto& [_purpose, _entry] : data) {
        if (_purpose != purpose) continue;
        entries_ptr = &_entry; break;
    }
    if (entries_ptr == nullptr) return std::nullopt;

    std::vector<DomainPort> ret{};
    for (const auto& entry : *entries_ptr) {
        if (auto service = entry.GetDomainPort(); service.has_value()) {
            ret.push_back(service.value());
        }
    }
    if (ret.empty()) return std::nullopt;

    return ret;
}

// Stub function to ensure compiler verifies serialization functions actually work
void TestSerializationMagic()
{
    MnNetInfo network_info;
    CDataStream s(SER_NETWORK, PROTOCOL_VERSION);
    s << network_info;
    s >> network_info;
}

// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/netinfo.h>

#include <chainparams.h>
#include <netbase.h>
#include <util/system.h>

namespace {
static std::unique_ptr<const CChainParams> g_main_params{nullptr};
static const CService empty_service{CService()};

static constexpr std::string_view SAFE_CHARS_IPV4{"1234567890."};
static constexpr std::string_view SAFE_CHARS_IPV4_6{"abcdefABCDEF1234567890.:[]"};
static constexpr std::string_view SAFE_CHARS_RFC1035{"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-"};

bool IsNodeOnMainnet() { return Params().NetworkIDString() == CBaseChainParams::MAIN; }
const CChainParams& MainParams()
{
    // TODO: use real args here
    if (!g_main_params) g_main_params = CreateChainParams(ArgsManager{}, CBaseChainParams::MAIN);
    return *g_main_params;
}

bool HasBadTLD(const std::string& str)
{
    const std::vector<std::string_view> blocklist{
        ".local",
        ".intranet",
        ".internal",
        ".private",
        ".corp",
        ".home",
        ".lan",
        ".home.arpa",
        ".onion",
        ".i2p"
    };
    for (const auto& tld : blocklist) {
        if (tld.size() > str.size()) continue;
        if (std::equal(tld.rbegin(), tld.rend(), str.rbegin())) return true;
    }
    return false;
}

bool IsAllowedPlatformHTTPPort(uint16_t port) {
    switch (port) {
    case 80:
    case 443:
        return true;
    }
    return false;
}

bool MatchCharsFilter(const std::string& input, const std::string_view& filter)
{
    for (char c : input) {
        if (filter.find(c) == std::string::npos) {
            return false;
        }
    }
    return true;
}
} // anonymous namespace

DomainPort::Status DomainPort::ValidateDomain(const std::string& addr)
{
    if (addr.length() > 253 || addr.length() < 4) {
        return DomainPort::Status::BadLen;
    }
    if (!MatchCharsFilter(addr, SAFE_CHARS_RFC1035)) {
        return DomainPort::Status::BadChar;
    }
    if (addr.at(0) == '.' || addr.at(addr.length() - 1) == '.') {
        return DomainPort::Status::BadCharPos;
    }
    std::vector<std::string> labels{SplitString(addr, '.')};
    if (labels.size() < 2) {
        return DomainPort::Status::BadDotless;
    }
    for (const auto& label : labels) {
        if (label.empty() || label.length() > 63) {
            return DomainPort::Status::BadLabelLen;
        }
        if (label.at(0) == '-' or label.at(label.length() - 1) == '-') {
            return DomainPort::Status::BadLabelCharPos;
        }
    }
    return DomainPort::Status::Success;
}

DomainPort::Status DomainPort::Set(const std::string& addr, const uint16_t port)
{
    if (port == 0) {
        return DomainPort::Status::BadPort;
    }
    const auto ret{ValidateDomain(addr)};
    if (ret == DomainPort::Status::Success) {
        // Convert to lowercase to avoid duplication by changing case (domains are case-insensitive)
        m_addr = ToLower(addr);
        m_port = port;
    }
    return ret;
}

DomainPort::Status DomainPort::Validate() const
{
    if (m_addr.empty() || m_addr != ToLower(m_addr)) {
        return DomainPort::Status::Malformed;
    }
    if (m_port == 0) {
        return DomainPort::Status::BadPort;
    }
    return ValidateDomain(m_addr);
}

bool NetInfoEntry::operator==(const NetInfoEntry& rhs) const
{
    if (m_type != rhs.m_type) return false;
    return std::visit([](auto&& lhs, auto&& rhs) -> bool {
        if constexpr (std::is_same_v<std::decay_t<decltype(lhs)>, std::decay_t<decltype(rhs)>>) {
            return lhs == rhs;
        }
        return false;
    }, m_data, rhs.m_data);
}

bool NetInfoEntry::operator<(const NetInfoEntry& rhs) const
{
    if (m_type != rhs.m_type) return m_type < rhs.m_type;
    return std::visit([](auto&& lhs, auto&& rhs) -> bool {
        using T1 = std::decay_t<decltype(lhs)>;
        using T2 = std::decay_t<decltype(rhs)>;
        if constexpr (std::is_same_v<T1, T2>) {
            // Both the same type, compare as usual
            return lhs < rhs;
        } else if constexpr ((std::is_same_v<T1, CService> || std::is_same_v<T1, DomainPort>)
                          && (std::is_same_v<T2, CService> || std::is_same_v<T2, DomainPort>)) {
            // Differing types but both implement ToStringAddrPort()
            return lhs.ToStringAddrPort() < rhs.ToStringAddrPort();
        } else if constexpr (std::is_same_v<T1, std::monostate> && !std::is_same_v<T2, std::monostate>) {
            // lhs is monostate and rhs is not, rhs is greater
            return true;
        } else if constexpr (!std::is_same_v<T1, std::monostate> && std::is_same_v<T2, std::monostate>) {
            // rhs is monostate but lhs is not, lhs is greater
            return false;
        }
        return false;
    }, m_data, rhs.m_data);
}

std::optional<std::reference_wrapper<const CService>> NetInfoEntry::GetAddrPort() const
{
    if (const auto* data_ptr{std::get_if<CService>(&m_data)}; data_ptr != nullptr && IsSupportedServiceType(m_type)) {
        return *data_ptr;
    }
    return std::nullopt;
}

std::optional<std::reference_wrapper<const DomainPort>> NetInfoEntry::GetDomainPort() const
{
    if (const auto* data_ptr{std::get_if<DomainPort>(&m_data)}; data_ptr != nullptr && IsTypeExtension(m_type)) {
        return *data_ptr;
    }
    return std::nullopt;
}

// NetInfoEntry is a dumb object that doesn't enforce validation rules, that is the responsibility of
// types that utilize NetInfoEntry (MnNetInfo and others). IsTriviallyValid() is there to check if a
// NetInfoEntry object is properly constructed.
bool NetInfoEntry::IsTriviallyValid() const
{
    if (m_type == INVALID_TYPE) return false;
    return std::visit([this](auto&& input) -> bool {
        using T1 = std::decay_t<decltype(input)>;
        if constexpr (std::is_same_v<T1, std::monostate>) {
            // Empty underlying data isn't a valid entry
            return false;
        } else if constexpr (std::is_same_v<T1, CService>) {
            // Type code should be truthful as it decides what underlying type is used when (de)serializing
            if (m_type != GetSupportedServiceType(input)) return false;
            // Underlying data should at least meet surface-level validity checks
            if (!input.IsValid()) return false;
            // Type code should be supported by NetInfoEntry
            if (!IsSupportedServiceType(m_type)) return false;
        } else if constexpr (std::is_same_v<T1, DomainPort>) {
            // Type code should be truthful as it decides what underlying type is used when (de)serializing
            if (m_type != Extensions::DOMAINS) return false;
            // Underlying data should at least meet surface-level validity checks
            if (input.Validate() != DomainPort::Status::Success) return false;
            // Type code should be supported by NetInfoEntry
            if (!IsTypeExtension(m_type)) return false;
        } else {
            return false;
        }
        return true;
    }, m_data);
}

std::string NetInfoEntry::ToString() const
{
    return std::visit([](auto&& input) -> std::string {
        using T1 = std::decay_t<decltype(input)>;
        if constexpr (std::is_same_v<T1, CService>) {
            return strprintf("CService(addr=%s, port=%d)", input.ToStringAddr(), input.GetPort());
        } else if constexpr (std::is_same_v<T1, DomainPort>) {
            return strprintf("DomainPort(addr=%s, port=%d)", input.ToStringAddr(), input.GetPort());
        } else {
            return strprintf("[invalid entry]");
        }
    }, m_data);
}

std::string NetInfoEntry::ToStringAddrPort() const
{
    return std::visit([](auto&& input) -> std::string {
        using T1 = std::decay_t<decltype(input)>;
        if constexpr (std::is_same_v<T1, CService> || std::is_same_v<T1, DomainPort>) {
            return input.ToStringAddrPort();
        } else {
            return strprintf("[invalid entry]");
        }
    }, m_data);
}

NetInfoStatus MnNetInfo::ValidateService(const CService& service)
{
    if (!service.IsValid()) {
        return NetInfoStatus::BadInput;
    }
    if (!service.IsIPv4()) {
        return NetInfoStatus::BadInput;
    }
    if (Params().RequireRoutableExternalIP() && !service.IsRoutable()) {
        return NetInfoStatus::BadInput;
    }

    const auto default_port_main = MainParams().GetDefaultPort();
    if (IsNodeOnMainnet() && service.GetPort() != default_port_main) {
        // Must use mainnet port on mainnet
        return NetInfoStatus::BadPort;
    } else if (service.GetPort() == default_port_main) {
        // Using mainnet port prohibited outside of mainnet
        return NetInfoStatus::BadPort;
    }

    return NetInfoStatus::Success;
}

NetInfoStatus MnNetInfo::AddEntry(const uint8_t purpose, const std::string& input)
{
    if (purpose != Purpose::CORE_P2P || !IsEmpty()) {
        return NetInfoStatus::MaxLimit;
    }

    std::string addr; uint16_t port{Params().GetDefaultPort()};
    SplitHostPort(input, port, addr);
    // Contains invalid characters, unlikely to pass Lookup(), fast-fail
    if (!MatchCharsFilter(addr, SAFE_CHARS_IPV4)) {
        return NetInfoStatus::BadInput;
    }

    if (auto service = Lookup(addr, /*portDefault=*/port, /*fAllowLookup=*/false); service.has_value()) {
        const auto ret = ValidateService(service.value());
        if (ret == NetInfoStatus::Success) {
            const auto candidate{NetInfoEntry(service.value())};
            if (candidate == m_addr) {
                // Not possible since we allow only one value at most
                return NetInfoStatus::Duplicate;
            }
            m_addr = candidate;
        }
        return ret;
    }
    return NetInfoStatus::BadInput;
}

NetInfoList MnNetInfo::GetEntries() const
{
    NetInfoList ret;
    if (!IsEmpty()) {
        ret.push_back(m_addr);
    }
    // If MnNetInfo is empty, we probably don't expect any entries to show up, so
    // we return a blank set instead.
    return ret;
}

const CService& MnNetInfo::GetPrimary() const
{
    if (const auto& service{m_addr.GetAddrPort()}; service.has_value()) {
        return *service;
    }
    return empty_service;
}

NetInfoStatus MnNetInfo::Validate() const
{
    if (!m_addr.IsTriviallyValid()) {
        return NetInfoStatus::Malformed;
    }
    return ValidateService(GetPrimary());
}

std::string MnNetInfo::ToString() const
{
    // Extra padding to account for padding done by the calling function.
    return strprintf("MnNetInfo()\n"
                     "    NetInfo(purpose=%s)\n"
                     "      %s\n", PurposeToString(Purpose::CORE_P2P), m_addr.ToString());
}

NetInfoStatus ExtNetInfo::ProcessCandidate(const uint8_t purpose, const NetInfoEntry& candidate)
{
    assert(candidate.IsTriviallyValid());

    if (const auto& all_entries{GetEntries()};
        std::find_if(all_entries.begin(), all_entries.end(), [candidate](const auto& obj){ return obj.get() == candidate; }) != all_entries.end()) {
        // We don't allow duplicate entries even *across* different lists
        return NetInfoStatus::Duplicate;
    }
    if (candidate.GetType() == Extensions::DOMAINS && purpose != Purpose::PLATFORM_HTTP) {
        // Domains only allowed for Platform HTTP(S) API
        return NetInfoStatus::BadInput;
    }
    if (auto it{m_data.find(purpose)}; it != m_data.end()) {
        // Existing entries list found, run some more sanity checks
        auto& [_, entries] = *it;
        if (entries.size() >= EXTNETINFO_ENTRIES_LIMIT) {
            return NetInfoStatus::MaxLimit;
        }
        entries.push_back(candidate);
        return NetInfoStatus::Success;
    } else {
        if (purpose == Purpose::CORE_P2P || purpose == Purpose::PLATFORM_P2P) {
            // The first entry may only be of the primary type
            if (candidate.GetType() != EXTNETINFO_PRIMARY_ADDR_TYPE) {
                return NetInfoStatus::BadInput;
            }
        }
        // First entry for purpose code, create new entries list
        auto [_, status] = m_data.try_emplace(purpose, std::vector<NetInfoEntry>({candidate}));
        assert(status); // We did just check to see if our value already existed, try_emplace shouldn't fail
        return NetInfoStatus::Success;
    }
}

NetInfoStatus ExtNetInfo::ValidateService(const CService& service)
{
    if (!service.IsValid()) {
        return NetInfoStatus::BadInput;
    }
    if (Params().RequireRoutableExternalIP() && !service.IsRoutable()) {
        return NetInfoStatus::BadInput;
    }
    if (!IsSupportedServiceType(GetSupportedServiceType(service))) {
        return NetInfoStatus::BadInput;
    }
    if (IsBadPort(service.GetPort()) || service.GetPort() == 0) {
        return NetInfoStatus::BadPort;
    }

    return NetInfoStatus::Success;
}

NetInfoStatus ExtNetInfo::ValidateDomainPort(const DomainPort& service)
{
    if ((IsBadPort(service.GetPort()) && !IsAllowedPlatformHTTPPort(service.GetPort())) || service.GetPort() == 0) {
        return NetInfoStatus::BadPort;
    }
    if (service.Validate() != DomainPort::Status::Success) {
        return NetInfoStatus::BadInput;
    }
    if (HasBadTLD(service.ToStringAddr())) {
        return NetInfoStatus::BadInput;
    }

    return NetInfoStatus::Success;
}

NetInfoStatus ExtNetInfo::AddEntry(const uint8_t purpose, const std::string& input)
{
    if (!IsValidPurpose(purpose)) {
        return NetInfoStatus::MaxLimit;
    }

    // We don't allow assuming ports, so we set the default value to 0 so that if no port is specified
    // it uses a fallback value of 0, which will return a NetInfoStatus::BadPort
    std::string addr; uint16_t port{0};
    SplitHostPort(input, port, addr);

    if (!MatchCharsFilter(addr, SAFE_CHARS_IPV4_6)) {
        if (!MatchCharsFilter(addr, SAFE_CHARS_RFC1035)) {
            // Neither IP:port safe nor domain-safe, we can safely assume it's bad input
            return NetInfoStatus::BadInput;
        }

        // Not IP:port safe but domain safe, treat as domain.
        if (DomainPort service; service.Set(addr, port) == DomainPort::Status::Success) {
            const auto ret = ValidateDomainPort(service);
            if (ret == NetInfoStatus::Success) {
                return ProcessCandidate(purpose, NetInfoEntry(service));
            }
            return ret; /* ValidateDomainPort() failed */
        }
        return NetInfoStatus::BadInput; /* DomainPort::Set() failed */
    }

    // IP:port safe, try to parse it as IP:port
    if (auto service = Lookup(addr, /*portDefault=*/port, /*fAllowLookup=*/false); service.has_value()) {
        const auto ret = ValidateService(service.value());
        if (ret == NetInfoStatus::Success) {
            return ProcessCandidate(purpose, NetInfoEntry(service.value()));
        }
        return ret; /* ValidateService() failed */
    }
    return NetInfoStatus::BadInput; /* Lookup() failed */
}

NetInfoList ExtNetInfo::GetEntries() const
{
    NetInfoList ret;
    for (const auto& [_, entries] : m_data) {
        ret.insert(ret.end(), entries.begin(), entries.end());
    }
    return ret;
}

const CService& ExtNetInfo::GetPrimary() const
{
    if (const auto& it{m_data.find(Purpose::CORE_P2P)}; it != m_data.end()) {
        const auto& [_, entries] = *it;
        if (!entries.empty()) {
            if (const auto& service{entries.begin()->GetAddrPort()}; service.has_value()) {
                return service.value();
            }
        }
    }
    return empty_service;
}

bool ExtNetInfo::HasEntries(uint8_t purpose) const
{
    if (!IsValidPurpose(purpose)) return false;
    const auto& it{m_data.find(purpose)};
    return it != m_data.end() && !it->second.empty();
}

NetInfoStatus ExtNetInfo::Validate() const
{
    if (m_version == 0 || m_version > EXTNETINFO_FORMAT_VERSION) {
        return NetInfoStatus::Malformed;
    }
    if (m_data.empty()) {
        return NetInfoStatus::Malformed;
    }
    {
        const auto& all_entries{GetEntries()};
        if (std::set<NetInfoEntry> set{all_entries.begin(), all_entries.end()}; set.size() != all_entries.size()) {
            // Duplicate entries not allowed *across* different lists
            return NetInfoStatus::Duplicate;
        }
    }
    for (const auto& [purpose, entries] : m_data) {
        if (!IsValidPurpose(purpose)) {
            // Invalid purpose code
            return NetInfoStatus::Malformed;
        }
        if (entries.empty()) {
            // Purpose if present in map must have at least one entry
            return NetInfoStatus::Malformed;
        }
        for (const auto& entry : entries) {
            if (!entry.IsTriviallyValid()) {
                // Trivially invalid NetInfoEntry, no point checking against consensus rules
                return NetInfoStatus::Malformed;
            }
            if (purpose == Purpose::CORE_P2P || purpose == Purpose::PLATFORM_P2P) {
                if (entry == *entries.begin() && entry.GetType() != EXTNETINFO_PRIMARY_ADDR_TYPE) {
                    // First entry must be of the primary type
                    return NetInfoStatus::Malformed;
                }
            }
            if (const auto& service{entry.GetAddrPort()}; service.has_value()) {
                if (auto ret{ValidateService(*service)}; ret != NetInfoStatus::Success) {
                    // Stores CService underneath but doesn't pass validation rules
                    return ret;
                }
            } else if (const auto& service{entry.GetDomainPort()}; service.has_value()) {
                if (purpose != Purpose::PLATFORM_HTTP) {
                    // Domains only allowed for Platform HTTP(S) API
                    return NetInfoStatus::BadInput;
                }
                if (auto ret{ValidateDomainPort(*service)}; ret != NetInfoStatus::Success) {
                    // Stores DomainPort underneath but doesn't pass validation rules
                    return ret;
                }
            } else {
                // Doesn't store valid type underneath
                return NetInfoStatus::Malformed;
            }
        }
    }
    return NetInfoStatus::Success;
}

std::string ExtNetInfo::ToString() const
{
    std::string ret{"ExtNetInfo()\n"};
    for (const auto& [purpose, entries] : m_data) {
        ret += strprintf("    NetInfo(purpose=%s)\n", PurposeToString(purpose));
        if (entries.empty()) {
            ret += strprintf("      [invalid list]\n");
        } else {
            for (const auto& entry : entries) {
                ret += strprintf("      %s\n", entry.ToString());
            }
        }
    }
    return ret;
}

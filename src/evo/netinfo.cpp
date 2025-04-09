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

static constexpr std::string_view SAFE_CHARS_IPPORT{"1234567890.[]:"};

const bool IsNodeOnMainnet() { return Params().NetworkIDString() == CBaseChainParams::MAIN; }
const CChainParams& MainParams()
{
    // TODO: use real args here
    if (!g_main_params) g_main_params = CreateChainParams(ArgsManager{}, CBaseChainParams::MAIN);
    return *g_main_params;
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
        } else {
            return false;
        }
        return true;
    }, m_data);
}

std::string NetInfoEntry::ToString() const
{
    return std::visit([this](auto&& input) -> std::string {
        using T1 = std::decay_t<decltype(input)>;
        if constexpr (std::is_same_v<T1, CService>) {
            return strprintf("CService(addr=%s, port=%d)", input.ToStringAddr(), input.GetPort());
        } else {
            return strprintf("[invalid entry]");
        }
    }, m_data);
}

std::string NetInfoEntry::ToStringAddrPort() const
{
    return std::visit([this](auto&& input) -> std::string {
        using T1 = std::decay_t<decltype(input)>;
        if constexpr (std::is_same_v<T1, CService>) {
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

NetInfoStatus MnNetInfo::AddEntry(const std::string& input)
{
    if (!IsEmpty()) {
        return NetInfoStatus::MaxLimit;
    }
    // Contains invalid characters, unlikely to pass Lookup(), fast-fail
    if (!MatchCharsFilter(input, SAFE_CHARS_IPPORT)) {
        return NetInfoStatus::BadInput;
    }
    if (auto service = Lookup(input, /*portDefault=*/Params().GetDefaultPort(), /*fAllowLookup=*/false); service.has_value()) {
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
                     "    %s\n", m_addr.ToString());
}

NetInfoStatus ExtNetInfo::ProcessCandidate(const NetInfoEntry& candidate)
{
    assert(candidate.IsTriviallyValid());

    if (m_data.empty()) {
        if (candidate.GetType() != EXTNETINFO_PRIMARY_ADDR_TYPE) {
            return NetInfoStatus::BadInput;
        }
    }
    if (m_data.size() >= EXTNETINFO_ENTRIES_LIMIT) {
        return NetInfoStatus::MaxLimit;
    }
    if (std::find(m_data.begin(), m_data.end(), candidate) != m_data.end()) {
        return NetInfoStatus::Duplicate;
    }
    m_data.push_back(candidate);

    return NetInfoStatus::Success;
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

NetInfoStatus ExtNetInfo::AddEntry(const std::string& input)
{
    // Contains invalid characters, unlikely to pass Lookup(), fast-fail
    if (!MatchCharsFilter(input, SAFE_CHARS_IPPORT)) {
        return NetInfoStatus::BadInput;
    }
    // We don't allow assuming ports, so we set portDefault to 0 so that if no port is specified
    // it uses a fallback value of 0, which will return a NetInfoStatus::BadPort from ValidateService()
    if (auto service = Lookup(input, /*portDefault=*/0, /*fAllowLookup=*/false); service.has_value()) {
        const auto ret = ValidateService(service.value());
        if (ret == NetInfoStatus::Success) {
            return ProcessCandidate(NetInfoEntry(service.value()));
        }
        return ret; /* ValidateService() failed */
    }
    return NetInfoStatus::BadInput; /* Lookup() failed */
}

NetInfoList ExtNetInfo::GetEntries() const
{
    NetInfoList ret;
    ret.insert(ret.end(), m_data.begin(), m_data.end());
    return ret;
}

const CService& ExtNetInfo::GetPrimary() const
{
    if (!m_data.empty()) {
        if (const auto& service{m_data.begin()->GetAddrPort()}; service.has_value()) {
            return service.value();
        }
    }
    return empty_service;
}

NetInfoStatus ExtNetInfo::Validate() const
{
    if (m_version == 0 || m_version > EXTNETINFO_FORMAT_VERSION) {
        return NetInfoStatus::Malformed;
    }
    if (m_data.empty()) {
        return NetInfoStatus::Malformed;
    }
    if (std::set<NetInfoEntry> set{m_data.begin(), m_data.end()}; set.size() != m_data.size()) {
        return NetInfoStatus::Duplicate;
    }
    for (const auto& entry : m_data) {
        if (!entry.IsTriviallyValid()) {
            // Trivially invalid NetInfoEntry, no point checking against consensus rules
            return NetInfoStatus::Malformed;
        }
        if (entry == *m_data.begin() && entry.GetType() != EXTNETINFO_PRIMARY_ADDR_TYPE) {
            // First entry must be of the primary type
            return NetInfoStatus::Malformed;
        }
        if (const auto& service{entry.GetAddrPort()}; service.has_value()) {
            if (auto ret{ValidateService(*service)}; ret != NetInfoStatus::Success) {
                // Stores CService underneath but doesn't pass validation rules
                return ret;
            }
        } else {
            // Doesn't store valid type underneath
            return NetInfoStatus::Malformed;
        }
    }
    return NetInfoStatus::Success;
}

std::string ExtNetInfo::ToString() const
{
    std::string ret{"ExtNetInfo()\n"};
    for (const auto& entry : m_data) {
        ret += strprintf("    %s\n", entry.ToString());
    }
    return ret;
}

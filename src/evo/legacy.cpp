// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/legacy.h>

#include <chainparams.h>
#include <util/system.h>

#include <univalue.h>

namespace {
static std::unique_ptr<const CChainParams> g_main_params{nullptr};

const bool IsNodeOnMainnet() { return Params().NetworkIDString() == CBaseChainParams::MAIN; }
const CChainParams& MainParams()
{
    // TODO: use real args here
    if (!g_main_params) g_main_params = CreateChainParams(ArgsManager{}, CBaseChainParams::MAIN);
    return *g_main_params;
}
} // anonymous namespace

MnNetStatus OldMnNetInfo::ValidateService(CService service)
{
    if (!service.IsValid()) {
        return MnNetStatus::BadInput;
    }
    if (!service.IsIPv4()) {
        return MnNetStatus::BadInput;
    }
    if (Params().RequireRoutableExternalIP() && !service.IsRoutable()) {
        return MnNetStatus::BadInput;
    }

    const auto default_port_main = MainParams().GetDefaultPort();
    if (IsNodeOnMainnet() && service.GetPort() != default_port_main) {
        return MnNetStatus::BadPort;
    } else if (service.GetPort() == default_port_main) {
        return MnNetStatus::BadPort;
    }

    return MnNetStatus::Success;
}

// Domain names aren't supported in legacy format
MnNetStatus OldMnNetInfo::AddEntry(Purpose purpose, DomainPort service)
{
    return MnNetStatus::MaxLimit;
}

MnNetStatus OldMnNetInfo::AddEntry(Purpose purpose, CService service)
{
    // Legacy format doesn't support anything other than storing Core P2P
    // addresses, so the maximum entries for everything else is 0.
    if (purpose != Purpose::CORE_P2P) {
        return MnNetStatus::MaxLimit;
    }

    // Legacy format doesn't support multiple entries
    if (!IsEmpty()) {
        return MnNetStatus::Duplicate;
    }

    const auto ret{ValidateService(service)};
    if (ret != MnNetStatus::Success) return ret;
    addr = service;
    return ret;
}

// Domain names aren't supported in legacy format
MnNetStatus OldMnNetInfo::RemoveEntry(DomainPort service)
{
    return MnNetStatus::NotFound;
}

// Implemented because interface assumes support for multiple entries.
MnNetStatus OldMnNetInfo::RemoveEntry(CService service)
{
    if (!IsEmpty() || service == CService()) {
        return MnNetStatus::NotFound;
    }
    if (service != addr) {
        return MnNetStatus::NotFound;
    }

    service = CService();
    return MnNetStatus::Success;
}

// The "primary" service is the service mandatory on all masternodes regardless of
// type. In legacy format, that's the *only* address but in the extended format, it
// will be the first entry of type CORE_P2P.
const CService& OldMnNetInfo::GetPrimaryService() const
{
    return addr;
}

std::vector<uint8_t> OldMnNetInfo::GetKey() const
{
    return addr.GetKey();
}

bool OldMnNetInfo::IsEmpty() const
{
    OldMnNetInfo rhs;
    rhs.Clear();
    return *this == rhs;
}

MnNetStatus OldMnNetInfo::Validate() const
{
    return ValidateService(addr);
}

void OldMnNetInfo::Clear()
{
    addr = CService();
}

UniValue OldMnNetInfo::ToJson() const
{
    UniValue ret(UniValue::VOBJ);

    // There's only one entry to consider so we do this instead of looping over
    // all entries as we would with the newer format
    auto make_arr = [](CService addr) {
        UniValue obj(UniValue::VARR);
        obj.push_back(addr.ToStringAddrPort()); // Should we separate the port and address?
        return obj;
    };

    UniValue core(UniValue::VOBJ);
    core.pushKV("p2p", make_arr(addr));

    // Segmenting core as a distinct object allows for future extensibility
    ret.pushKV("core", core);
    return ret;
}

std::string OldMnNetInfo::ToString() const
{
    // There's some extra padding to account for padding on the first line done by the calling function.
    return strprintf("MnNetInfo()\n"
    // We're faking support for multiple purposes so that the ToString formatting remains more-or-less
    // consistent when we switch over to the extended format.
                     "    NetInfo(purpose=CORE_P2P)\n"
                     "      CService(ip=%s, port=%u)\n", addr.ToStringAddr(), addr.GetPort());
}

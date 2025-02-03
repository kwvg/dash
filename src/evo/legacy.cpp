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

MnNetStatus OldMnNetInfo::AddEntry(CService service)
{
    if (!IsEmpty()) {
        return MnNetStatus::Duplicate;
    }

    const auto ret{ValidateService(service)};
    if (ret != MnNetStatus::Success) return ret;
    addr = service;
    return ret;
}

MnNetStatus OldMnNetInfo::RemoveEntry(CService service)
{
    if (!IsEmpty() || service == CService()) {
        // Should probably use Clear() if we want a full reset
        return MnNetStatus::NotFound;
    }
    if (service != addr) {
        return MnNetStatus::NotFound;
    }

    service = CService();
    return MnNetStatus::Success;
}

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
                     "    CService(ip=%s, port=%u)\n", addr.ToStringAddr(), addr.GetPort());
}

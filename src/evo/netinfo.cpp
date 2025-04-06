// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/netinfo.h>

#include <chainparams.h>
#include <netbase.h>
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

UniValue ArrFromService(CService addr)
{
    UniValue obj(UniValue::VARR);
    obj.push_back(addr.ToStringAddrPort());
    return obj;
}

bool IsServiceDeprecatedRPCEnabled()
{
    const auto args = gArgs.GetArgs("-deprecatedrpc");
    return std::find(args.begin(), args.end(), "service") != args.end();
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

NetInfoStatus MnNetInfo::AddEntry(const Purpose purpose, const std::string& input)
{
    // We only support storing Core P2P addresses, so the max limit for any other
    // purpose is 0 but even if it's a Core P2P address, we only support one entry,
    // so if we have anything stored already, we're full.
    if (purpose != Purpose::CORE_P2P || !IsEmpty()) {
        return NetInfoStatus::MaxLimit;
    }
    if (auto service = Lookup(input, /*portDefault=*/Params().GetDefaultPort(), /*fAllowLookup=*/false); service.has_value()) {
        const auto ret = ValidateService(service.value());
        if (ret == NetInfoStatus::Success) {
            if (service == addr) {
                // Not possible since we allow only one value at most
                return NetInfoStatus::Duplicate;
            }
            addr = service.value();
        }
        return ret;
    }
    return NetInfoStatus::BadInput;
}

CServiceList MnNetInfo::GetEntries() const
{
    CServiceList ret;
    if (!IsEmpty()) {
        ret.push_back(addr);
    }
    // If CService is empty, we probably want to skip code that expects valid
    // entries to iterate through, so we return a blank set instead.
    return ret;
}

UniValue MnNetInfo::ToJson() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV(PurposeToString(Purpose::CORE_P2P, /*lower=*/true), ArrFromService(addr));
    return ret;
}

std::string MnNetInfo::ToString() const
{
    return strprintf("MnNetInfo()\n"
    // Extra padding to account for padding done by the calling function.
                     "    NetInfo(purpose=%s)\n"
                     "      CService(addr=%s, port=%d)\n",
                     PurposeToString(Purpose::CORE_P2P), addr.ToStringAddr(), addr.GetPort());
}

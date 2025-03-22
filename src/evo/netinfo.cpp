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
        /* Must use mainnet port on mainnet */
        return NetInfoStatus::BadPort;
    } else if (service.GetPort() == default_port_main) {
        /* Using mainnet port prohibited outside of mainnet */
        return NetInfoStatus::BadPort;
    }

    return NetInfoStatus::Success;
}

NetInfoStatus MnNetInfo::SetEntry(const std::string input)
{
    if (auto service = Lookup(input, /*portDefault=*/Params().GetDefaultPort(), /*fAllowLookup=*/false); service.has_value()) {
        const auto ret = ValidateService(service.value());
        if (ret == NetInfoStatus::Success) {
            addr = service.value();
        }
        return ret;
    }
    return NetInfoStatus::BadInput;
}

UniValue MnNetInfo::ToJson() const
{
    UniValue ret(UniValue::VOBJ);

    /* There's only one entry to consider so there's nothing to iterate through */
    auto make_arr = [](CService addr) {
        UniValue obj(UniValue::VARR);
        obj.push_back(addr.ToStringAddrPort());
        return obj;
    };

    UniValue core(UniValue::VOBJ);
    core.pushKV("p2p", make_arr(addr));

    /* Segmenting core as a distinct object allows for future extensibility */
    ret.pushKV("core", core);
    return ret;
}

std::string MnNetInfo::ToString() const
{
    /* Extra padding to account for padding done by the calling function. */
    return strprintf("MnNetInfo()\n"
                     "    CService(ip=%s, port=%u)\n", addr.ToStringAddr(), addr.GetPort());
}

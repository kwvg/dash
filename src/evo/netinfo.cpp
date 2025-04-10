// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/netinfo.h>

#include <chainparams.h>
#include <netbase.h>
#include <util/system.h>

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
    if (auto service = Lookup(input, /*portDefault=*/Params().GetDefaultPort(), /*fAllowLookup=*/false); service.has_value()) {
        const auto ret = ValidateService(service.value());
        if (ret == NetInfoStatus::Success) {
            m_addr = service.value();
        }
        return ret;
    }
    return NetInfoStatus::BadInput;
}

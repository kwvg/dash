// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/netinfo.h>

#include <chainparams.h>
#include <netbase.h>

NetInfoStatus MnNetInfo::AddEntry(const std::string input)
{
    if (auto service = Lookup(input, /*portDefault=*/Params().GetDefaultPort(), /*fAllowLookup=*/false); service.has_value()) {
        addr = service.value();
        return NetInfoStatus::Success;
    }
    return NetInfoStatus::BadInput;
}

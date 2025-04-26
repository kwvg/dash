// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/evo_util.h>

#include <evo/netinfo.h>
#include <evo/providertx.h>
#include <rpc/util.h>
#include <rpc/protocol.h>
#include <rpc/request.h>
#include <util/check.h>

#include <univalue.h>

template <typename T1>
void ProcessNetInfoCore(T1& ptx, const UniValue& input, const bool optional)
{
    CHECK_NONFATAL(ptx.netInfo);

    if (!input.isStr()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid param for ipAndPort, must be string");
    }
    if (!optional && input.get_str().empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Empty param for ipAndPort not allowed");
    }
    if (!input.get_str().empty()) {
        if (auto entryRet = ptx.netInfo->AddEntry(Purpose::CORE_P2P, input.get_str()); entryRet != NetInfoStatus::Success) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Error setting ipAndPort to '%s' (%s)", input.get_str(), NISToString(entryRet)));
        }
    }
}
template void ProcessNetInfoCore(CProRegTx& ptx, const UniValue& input, const bool optional);
template void ProcessNetInfoCore(CProUpServTx& ptx, const UniValue& input, const bool optional);

template <typename T1>
void ProcessNetInfoPlatform(T1& ptx, const UniValue& input_p2p, const UniValue& input_http)
{
    CHECK_NONFATAL(ptx.netInfo);

    if (!input_p2p.isNum() && !input_p2p.isStr()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid param for platformP2PPort, must be number");
    }
    if (int32_t port{ParseInt32V(input_p2p, "platformP2PPort")}; port >= 1 && port <= std::numeric_limits<uint16_t>::max()) {
        ptx.platformP2PPort = static_cast<uint16_t>(port);
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "platformP2PPort must be a valid port [1-65535]");
    }

    if (!input_http.isNum() && !input_http.isStr()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid param for platformHTTPPort, must be number");
    }
    if (int32_t port{ParseInt32V(input_http, "platformHTTPPort")}; port >= 1 && port <= std::numeric_limits<uint16_t>::max()) {
        ptx.platformHTTPPort = static_cast<uint16_t>(port);
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "platformHTTPPort must be a valid port [1-65535]");
    }
}
template void ProcessNetInfoPlatform(CProRegTx& ptx, const UniValue& input_p2p, const UniValue& input_http);
template void ProcessNetInfoPlatform(CProUpServTx& ptx, const UniValue& input_p2p, const UniValue& input_http);

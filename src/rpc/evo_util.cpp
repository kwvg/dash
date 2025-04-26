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

namespace {
bool IsNumeric(const std::string_view& input)
{
    return input.find_first_not_of("0123456789") == std::string::npos;
}
} // anonymous namespace

template <typename T1>
void ProcessNetInfoCore(T1& ptx, const UniValue& input, const bool optional)
{
    CHECK_NONFATAL(ptx.netInfo);

    if (!input.isArray() && !input.isStr()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid param for coreP2PAddrs, must be string or array");
    }

    if (input.isStr()) {
        if (!optional && input.get_str().empty()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Empty param for coreP2PAddrs not allowed");
        } else if (!input.get_str().empty()) {
            if (auto entryRet = ptx.netInfo->AddEntry(Purpose::CORE_P2P, input.get_str()); entryRet != NetInfoStatus::Success) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Error setting coreP2PAddrs[0] to '%s' (%s)", input.get_str(), NISToString(entryRet)));
            }
        }
    } else if (input.isArray()) {
        if (!optional && input.get_array().empty()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Empty params for coreP2PAddrs not allowed");
        } else if (!input.get_array().empty()) {
            const UniValue& entries = input.get_array();
            for (size_t idx{0}; idx < entries.size(); idx++) {
                const UniValue& entry{entries[idx]};
                if (!entry.isStr()) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid param for coreP2PAddrs[%d], must be string", idx));
                }
                if (auto entryRet = ptx.netInfo->AddEntry(Purpose::CORE_P2P, entry.get_str()); entryRet != NetInfoStatus::Success) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Error setting coreP2PAddrs[%d] to '%s' (%s)", idx, entry.get_str(), NISToString(entryRet)));
                }
            }
        }
    }
}
template void ProcessNetInfoCore(CProRegTx& ptx, const UniValue& input, const bool optional);
template void ProcessNetInfoCore(CProUpServTx& ptx, const UniValue& input, const bool optional);

template <typename T1>
void ProcessNetInfoPlatform(T1& ptx, const UniValue& input_p2p, const UniValue& input_http, const bool optional)
{
    CHECK_NONFATAL(ptx.netInfo);

    auto process_field = [&](uint16_t& maybe_target, const UniValue& input, const uint8_t& purpose, const std::string& field_name) {
        if (!input.isNum() && !input.isStr()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid param for %s, must be number or string", field_name));
        }
        if (input.getValStr().empty()) {
            if (!optional) {
                // Mandatory field, cannot specify blank value
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Empty param for %s not allowed", field_name));
            } else {
                if (!ptx.netInfo->CanStorePlatform()) {
                    // We can tolerate blank values if netInfo can store platform fields, if it cannot, we are relying on
                    // platform{HTTP,P2P}Port, where it is mandatory even if their netInfo counterpart is optional.
                    throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("ProTx version disallows storing blank values in %s (must specify port number)", field_name));
                } else if (!ptx.netInfo->IsEmpty()) {
                    // Blank values are tolerable so long as no other field has been populated.
                    throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Cannot leave %s empty if other address fields populated", field_name));
                }
            }
            // Blank value permitted, bail out
            return;
        }
        if (!IsNumeric(input.getValStr())) {
            // Cannot be parsed as a number (port) so must be an addr:port string
            if (!ptx.netInfo->CanStorePlatform()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("ProTx version disallows storing addresses in %s (must specify port number only)", field_name));
            }
            if (auto entryRet = ptx.netInfo->AddEntry(purpose, input.get_str()); entryRet != NetInfoStatus::Success) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Error setting %s[0] to '%s' (%s)", field_name, input.get_str(), NISToString(entryRet)));
            }
        } else if (int32_t port{0}; ParseInt32(input.getValStr(), &port) && port >= 1 && port <= std::numeric_limits<uint16_t>::max()) {
            // Valid port
            if (!ptx.netInfo->CanStorePlatform()) {
                maybe_target = static_cast<uint16_t>(port);
            } else {
                // We cannot store *only* a port number in netInfo so we need to associate it with the primary service of CORE_P2P manually
                if (!ptx.netInfo->HasEntries(Purpose::CORE_P2P)) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Must specify coreP2PAddrs in order to set %s", field_name));
                }
                const CService service{CNetAddr{ptx.netInfo->GetPrimary()}, static_cast<uint16_t>(port)};
                CHECK_NONFATAL(service.IsValid());
                if (auto entryRet = ptx.netInfo->AddEntry(purpose, service.ToStringAddrPort()); entryRet != NetInfoStatus::Success) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Error setting %s[0] to '%s' (%s)", field_name, service.ToStringAddrPort(), NISToString(entryRet)));
                }
            }
        } else {
            // Invalid port
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s must be a valid port [1-65535]", field_name));
        }
    };
    process_field(ptx.platformP2PPort, input_p2p, Purpose::PLATFORM_P2P, "platformP2PAddrs");
    process_field(ptx.platformHTTPPort, input_http, Purpose::PLATFORM_HTTP, "platformHTTPAddrs");
}
template void ProcessNetInfoPlatform(CProRegTx& ptx, const UniValue& input_p2p, const UniValue& input_http, const bool optional);
template void ProcessNetInfoPlatform(CProUpServTx& ptx, const UniValue& input_p2p, const UniValue& input_http, const bool optional);

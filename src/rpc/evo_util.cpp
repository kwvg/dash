// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/evo_util.h>

#include <evo/deterministicmns.h>
#include <evo/netinfo.h>
#include <evo/providertx.h>
#include <evo/simplifiedmns.h>
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

template <typename T1>
inline UniValue NetInfoJsonInternal(const T1& obj, const MnType& type)
{
    UniValue ret{obj.netInfo->ToJson()};
    if (obj.netInfo->CanStorePlatform() || type != MnType::Evo) return ret;
    const CService& addr{obj.netInfo->GetPrimary()};
    ret.pushKV(PurposeToString(Purpose::PLATFORM_HTTP, /*lower=*/true), ArrFromService(CService(CNetAddr{addr}, obj.platformHTTPPort)));
    if constexpr (!std::is_same<T1, CSimplifiedMNListEntry>::value) /* CSimplifiedMNListEntry doesn't have this field */ {
        ret.pushKV(PurposeToString(Purpose::PLATFORM_P2P, /*lower=*/true), ArrFromService(CService(CNetAddr{addr}, obj.platformP2PPort)));
    }
    return ret;
}

template <typename T1>
inline int32_t GetPlatformPortInternal(const T1& obj, const bool is_http, const MnType& type)
{
    CHECK_NONFATAL(type == MnType::Evo);

    const int32_t err_ret{-1};
    if (!obj.netInfo->CanStorePlatform()) {
        // The port is stored in dedicated fields, just return those
        if (is_http) { return obj.platformHTTPPort; }
        if constexpr (!std::is_same<T1, CSimplifiedMNListEntry>::value) /* CSimplifiedMNListEntry doesn't have this field */ {
            if (!is_http) { return obj.platformP2PPort; }
        }
        return err_ret;
    }

    // We can only retrieve the port *if* there is a PLATFORM_{HTTP,P2P} entry that shares the same address as
    // CORE_P2P's first entry, otherwise give up.
    CNetAddr primary_addr{obj.netInfo->GetPrimary()};
    CHECK_NONFATAL(primary_addr.IsValid());
    for (const NetInfoEntry& entry : obj.netInfo->GetEntries(is_http ? Purpose::PLATFORM_HTTP : Purpose::PLATFORM_P2P)) {
        if (const auto& service_opt{entry.GetAddrPort()}; service_opt.has_value()) {
            if (const CNetAddr addr{service_opt.value()}; addr == primary_addr) {
                CHECK_NONFATAL(addr.IsValid());
                return service_opt.value().get().GetPort();
            }
        }
    }
    return err_ret;
}
} // anonymous namespace

int32_t GetPlatformHTTPPort(const CProRegTx& obj) { return GetPlatformPortInternal(obj, /*is_http=*/true, obj.nType); }
int32_t GetPlatformHTTPPort(const CProUpServTx& obj) { return GetPlatformPortInternal(obj, /*is_http=*/true, obj.nType); }
int32_t GetPlatformHTTPPort(const CDeterministicMNState& obj, const MnType& type) { return GetPlatformPortInternal(obj, /*is_http=*/true, type); }
int32_t GetPlatformHTTPPort(const CSimplifiedMNListEntry& obj, const MnType& type) { return GetPlatformPortInternal(obj, /*is_http=*/true, type); }

int32_t GetPlatformP2PPort(const CProRegTx& obj) { return GetPlatformPortInternal(obj, /*is_http=*/false, obj.nType); }
int32_t GetPlatformP2PPort(const CProUpServTx& obj) { return GetPlatformPortInternal(obj, /*is_http=*/false, obj.nType); }
int32_t GetPlatformP2PPort(const CDeterministicMNState& obj, const MnType& type) { return GetPlatformPortInternal(obj, /*is_http=*/false, type); }

UniValue NetInfoJson(const CProRegTx& obj) { return NetInfoJsonInternal(obj, obj.nType); }
UniValue NetInfoJson(const CProUpServTx& obj) { return NetInfoJsonInternal(obj, obj.nType); }
UniValue NetInfoJson(const CDeterministicMNState& obj, const MnType& type) { return NetInfoJsonInternal(obj, type); }
UniValue NetInfoJson(const CSimplifiedMNListEntry& obj, const MnType& type) { return NetInfoJsonInternal(obj, type); }

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
        if (!input.isArray() && !input.isNum() && !input.isStr()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid param for %s, must be array, number or string", field_name));
        }
        if ((input.isArray() && input.get_array().empty()) || ((input.isNum() || input.isStr()) && input.getValStr().empty())) {
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
        if (input.isArray()) {
            CHECK_NONFATAL(!input.get_array().empty());
            // Arrays are expected to be of address strings. If storing addresses aren't supported, bail out.
            if (!ptx.netInfo->CanStorePlatform()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("ProTx version disallows storing addresses in %s (must specify port number only)", field_name));
            }
            const UniValue& entries = input.get_array();
            for (size_t idx{0}; idx < entries.size(); idx++) {
                const UniValue& entry{entries[idx]};
                if (!entry.isStr() || IsNumeric(entry.get_str())) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid param for %s[%d], must be string", field_name, idx));
                }
                if (auto entryRet = ptx.netInfo->AddEntry(purpose, entry.get_str()); entryRet != NetInfoStatus::Success) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Error setting %s[%d] to '%s' (%s)", field_name, idx, entry.get_str(), NISToString(entryRet)));
                }
            }
            // Subsequent code is for strings and numbers, our work is done. Exit.
            return;
        }

        CHECK_NONFATAL(input.isNum() || input.isStr());

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

// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_EVO_UTIL_H
#define BITCOIN_RPC_EVO_UTIL_H

#include <cstdint>

class CDeterministicMNState;
class CProRegTx;
class CProUpServTx;
class CSimplifiedMNListEntry;
enum class MnType : uint16_t;

class UniValue;

/* Returns netInfo::GetJson() with data from platform fields. */
UniValue NetInfoJson(const CProRegTx& obj);
UniValue NetInfoJson(const CProUpServTx& obj);
UniValue NetInfoJson(const CDeterministicMNState& obj, const MnType& type);
UniValue NetInfoJson(const CSimplifiedMNListEntry& obj, const MnType& type);

template <typename T1>
void ProcessNetInfoCore(T1& ptx, const UniValue& input, const bool optional);

template <typename T1>
void ProcessNetInfoPlatform(T1& ptx, const UniValue& input_p2p, const UniValue& input_http, const bool optional);

#endif // BITCOIN_RPC_EVO_UTIL_H

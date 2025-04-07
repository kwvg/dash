// Copyright (c) 2018-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/dmnstate.h>
#include <evo/providertx.h>

#include <chainparams.h>
#include <consensus/validation.h>
#include <script/standard.h>
#include <validationinterface.h>

#include <univalue.h>
#include <messagesigner.h>

std::string CDeterministicMNState::ToString() const
{
    CTxDestination dest;
    std::string payoutAddress = "unknown";
    std::string operatorPayoutAddress = "none";
    if (ExtractDestination(scriptPayout, dest)) {
        payoutAddress = EncodeDestination(dest);
    }
    if (ExtractDestination(scriptOperatorPayout, dest)) {
        operatorPayoutAddress = EncodeDestination(dest);
    }

    return strprintf("CDeterministicMNState(nVersion=%d, nRegisteredHeight=%d, nLastPaidHeight=%d, nPoSePenalty=%d, "
                     "nPoSeRevivedHeight=%d, nPoSeBanHeight=%d, nRevocationReason=%d, "
                     "ownerAddress=%s, pubKeyOperator=%s, votingAddress=%s, payoutAddress=%s, "
                     "operatorPayoutAddress=%s)\n"
                     "  %s",
                     nVersion, nRegisteredHeight, nLastPaidHeight, nPoSePenalty, nPoSeRevivedHeight, nPoSeBanHeight,
                     nRevocationReason, EncodeDestination(PKHash(keyIDOwner)), pubKeyOperator.ToString(),
                     EncodeDestination(PKHash(keyIDVoting)), payoutAddress, operatorPayoutAddress, netInfo->ToString());
}

UniValue CDeterministicMNState::ToJson(MnType nType) const
{
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("version", nVersion);
    if (IsServiceDeprecatedRPCEnabled()) {
        obj.pushKV("service", netInfo->GetPrimary().ToStringAddrPort());
    }
    obj.pushKV("addresses", MaybeAddPlatformNetInfo(*this, nType, netInfo->ToJson()));
    obj.pushKV("registeredHeight", nRegisteredHeight);
    obj.pushKV("lastPaidHeight", nLastPaidHeight);
    obj.pushKV("consecutivePayments", nConsecutivePayments);
    obj.pushKV("PoSePenalty", nPoSePenalty);
    obj.pushKV("PoSeRevivedHeight", nPoSeRevivedHeight);
    obj.pushKV("PoSeBanHeight", nPoSeBanHeight);
    obj.pushKV("revocationReason", nRevocationReason);
    obj.pushKV("ownerAddress", EncodeDestination(PKHash(keyIDOwner)));
    obj.pushKV("votingAddress", EncodeDestination(PKHash(keyIDVoting)));
    if (nType == MnType::Evo) {
        obj.pushKV("platformNodeID", platformNodeID.ToString());
        if (IsServiceDeprecatedRPCEnabled()) {
            obj.pushKV("platformP2PPort", platformP2PPort);
            obj.pushKV("platformHTTPPort", platformHTTPPort);
        }
    }

    CTxDestination dest;
    if (ExtractDestination(scriptPayout, dest)) {
        obj.pushKV("payoutAddress", EncodeDestination(dest));
    }
    obj.pushKV("pubKeyOperator", pubKeyOperator.ToString());
    if (ExtractDestination(scriptOperatorPayout, dest)) {
        obj.pushKV("operatorPayoutAddress", EncodeDestination(dest));
    }
    return obj;
}

UniValue CDeterministicMNStateDiff::ToJson(MnType nType) const
{
    UniValue obj(UniValue::VOBJ);
    if (fields & Field_nVersion) {
        obj.pushKV("version", state.nVersion);
    }
    if (fields & Field_netInfo) {
        if (IsServiceDeprecatedRPCEnabled()) {
            obj.pushKV("service", state.netInfo->GetPrimary().ToStringAddrPort());
        }
    }
    if (fields & Field_nRegisteredHeight) {
        obj.pushKV("registeredHeight", state.nRegisteredHeight);
    }
    if (fields & Field_nLastPaidHeight) {
        obj.pushKV("lastPaidHeight", state.nLastPaidHeight);
    }
    if (fields & Field_nConsecutivePayments) {
        obj.pushKV("consecutivePayments", state.nConsecutivePayments);
    }
    if (fields & Field_nPoSePenalty) {
        obj.pushKV("PoSePenalty", state.nPoSePenalty);
    }
    if (fields & Field_nPoSeRevivedHeight) {
        obj.pushKV("PoSeRevivedHeight", state.nPoSeRevivedHeight);
    }
    if (fields & Field_nPoSeBanHeight) {
        obj.pushKV("PoSeBanHeight", state.nPoSeBanHeight);
    }
    if (fields & Field_nRevocationReason) {
        obj.pushKV("revocationReason", state.nRevocationReason);
    }
    if (fields & Field_keyIDOwner) {
        obj.pushKV("ownerAddress", EncodeDestination(PKHash(state.keyIDOwner)));
    }
    if (fields & Field_keyIDVoting) {
        obj.pushKV("votingAddress", EncodeDestination(PKHash(state.keyIDVoting)));
    }
    if (fields & Field_scriptPayout) {
        CTxDestination dest;
        if (ExtractDestination(state.scriptPayout, dest)) {
            obj.pushKV("payoutAddress", EncodeDestination(dest));
        }
    }
    if (fields & Field_scriptOperatorPayout) {
        CTxDestination dest;
        if (ExtractDestination(state.scriptOperatorPayout, dest)) {
            obj.pushKV("operatorPayoutAddress", EncodeDestination(dest));
        }
    }
    if (fields & Field_pubKeyOperator) {
        obj.pushKV("pubKeyOperator", state.pubKeyOperator.ToString());
    }
    if (nType == MnType::Evo) {
        if (fields & Field_platformNodeID) {
            obj.pushKV("platformNodeID", state.platformNodeID.ToString());
        }
        if (IsServiceDeprecatedRPCEnabled() && (fields & Field_platformP2PPort)) {
            obj.pushKV("platformP2PPort", state.platformP2PPort);
        }
        if (IsServiceDeprecatedRPCEnabled() && (fields & Field_platformHTTPPort)) {
            obj.pushKV("platformHTTPPort", state.platformHTTPPort);
        }
    }
    {
        UniValue netInfoObj(UniValue::VOBJ);
        if (fields & Field_netInfo) {
            netInfoObj = state.netInfo->ToJson();
        }
        if (nType == MnType::Evo) {
            auto unknownAddr = [](uint16_t port) -> UniValue {
                UniValue obj(UniValue::VARR);
                 // We don't know what the address is because it wasn't changed in the
                 // diff but we still need to report the port number
                obj.push_back(strprintf("255.255.255.255:%d", port));
                return obj;
            };
            if (fields & Field_platformP2PPort) {
                netInfoObj.pushKV(
                    PurposeToString(Purpose::PLATFORM_P2P, /*lower=*/true),
                    (fields & Field_netInfo) ? ArrFromService(CService(state.netInfo->GetPrimary(), state.platformP2PPort))
                                             : unknownAddr(state.platformP2PPort)
                );
            }
            if (fields & Field_platformHTTPPort) {
                netInfoObj.pushKV(
                    PurposeToString(Purpose::PLATFORM_HTTP, /*lower=*/true),
                    (fields & Field_netInfo) ? ArrFromService(CService(state.netInfo->GetPrimary(), state.platformHTTPPort))
                                             : unknownAddr(state.platformHTTPPort)
                );
            }
        }
        if (!netInfoObj.empty()) {
            obj.pushKV("addresses", netInfoObj);
        }
    }
    return obj;
}

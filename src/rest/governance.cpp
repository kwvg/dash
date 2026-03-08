// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rest/governance.h>

#include <context.h>
#include <core_io.h>
#include <evo/deterministicmns.h>
#include <governance/governance.h>
#include <governance/object.h>
#include <node/context.h>
#include <rest/common.h>
#include <util/check.h>

#include <drogon/HttpAppFramework.h>

#include <univalue.h>

#include <algorithm>
#include <string>
#include <vector>

using node::NodeContext;
using rest::ParseAcceptFormat;
using rest::WriteJsonReply;

namespace {
UniValue ProposalToJson(const CGovernanceObject& govObj, const CDeterministicMNList& tip_mn_list)
{
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("hash", govObj.GetHash().ToString());
    obj.pushKV("collateralHash", govObj.GetCollateralHash().ToString());

    UniValue data = govObj.GetJSONObject();

    auto pushStr = [&](const std::string& key, const std::string& dataKey) {
        if (data.exists(dataKey) && !data[dataKey].isNull()) {
            obj.pushKV(key, data[dataKey].get_str());
        } else {
            obj.pushKV(key, UniValue{});
        }
    };

    obj.pushKV("name", data.exists("name") ? data["name"].get_str() : "");
    pushStr("url", "url");
    pushStr("paymentAddress", "payment_address");

    if (data.exists("payment_amount")) {
        int64_t amount{0};
        if (ParseFixedPoint(data["payment_amount"].get_str(), 8, &amount)) {
            obj.pushKV("paymentAmount", ValueFromAmount(amount));
        } else {
            obj.pushKV("paymentAmount", ValueFromAmount(0));
        }
    } else {
        obj.pushKV("paymentAmount", ValueFromAmount(0));
    }

    if (data.exists("start_epoch")) {
        obj.pushKV("startEpoch", LocaleIndependentAtoi<int64_t>(data["start_epoch"].get_str()));
    } else {
        obj.pushKV("startEpoch", UniValue{});
    }
    if (data.exists("end_epoch")) {
        obj.pushKV("endEpoch", LocaleIndependentAtoi<int64_t>(data["end_epoch"].get_str()));
    } else {
        obj.pushKV("endEpoch", UniValue{});
    }

    obj.pushKV("creationTime", govObj.GetCreationTime());

    obj.pushKV("yesCount", govObj.GetYesCount(tip_mn_list, VOTE_SIGNAL_FUNDING));
    obj.pushKV("noCount", govObj.GetNoCount(tip_mn_list, VOTE_SIGNAL_FUNDING));
    obj.pushKV("abstainCount", govObj.GetAbstainCount(tip_mn_list, VOTE_SIGNAL_FUNDING));
    obj.pushKV("funded", govObj.IsSetCachedFunding());
    obj.pushKV("valid", govObj.IsSetCachedValid());

    return obj;
}

void rest_governance_proposals(const CoreContext& context,
                               const drogon::HttpRequestPtr& req,
                               rest::Callback&& cb)
{
    if (!rest::CheckWarmup(cb)) return;
    if (ParseAcceptFormat(req) != RESTResponseFormat::JSON) {
        rest::RESTERR(cb, drogon::k406NotAcceptable, "output format not found (available: json)");
        return;
    }

    const NodeContext* node = rest::GetNodeContext(context, cb);
    if (!node) return;

    auto tip_mn_list = CHECK_NONFATAL(node->dmnman)->GetListAtChainTip();

    std::vector<CGovernanceObject> objs;
    CHECK_NONFATAL(node->govman)->GetAllNewerThan(objs, 0);

    std::vector<const CGovernanceObject*> proposals;
    for (const auto& obj : objs) {
        if (obj.GetObjectType() != GovernanceObject::PROPOSAL) continue;
        if (!obj.IsSetCachedValid()) continue;
        proposals.push_back(&obj);
    }

    std::sort(proposals.begin(), proposals.end(),
              [](const CGovernanceObject* a, const CGovernanceObject* b) {
                  return a->GetCreationTime() > b->GetCreationTime();
              });

    UniValue arr(UniValue::VARR);
    for (const auto* p : proposals) {
        arr.push_back(ProposalToJson(*p, tip_mn_list));
    }

    WriteJsonReply(cb, arr);
}

void rest_governance_proposal(const CoreContext& context,
                              const drogon::HttpRequestPtr& req,
                              rest::Callback&& cb,
                              const std::string& hash_str)
{
    if (!rest::CheckWarmup(cb)) return;
    if (ParseAcceptFormat(req) != RESTResponseFormat::JSON) {
        rest::RESTERR(cb, drogon::k406NotAcceptable, "output format not found (available: json)");
        return;
    }

    const NodeContext* node = rest::GetNodeContext(context, cb);
    if (!node) return;

    uint256 hash;
    if (!ParseHashStr(hash_str, hash)) {
        rest::RESTERR(cb, drogon::k400BadRequest, "Invalid hash: " + hash_str);
        return;
    }

    auto pGovObj = CHECK_NONFATAL(node->govman)->FindConstGovernanceObject(hash);
    if (!pGovObj || pGovObj->GetObjectType() != GovernanceObject::PROPOSAL) {
        rest::RESTERR(cb, drogon::k404NotFound, hash_str + " not found");
        return;
    }

    auto tip_mn_list = CHECK_NONFATAL(node->dmnman)->GetListAtChainTip();
    WriteJsonReply(cb, ProposalToJson(*pGovObj, tip_mn_list));
}
} // anonymous namespace

namespace rest {
void RegisterGovernanceHandlers(const CoreContext& context, drogon::HttpAppFramework& app)
{
    app.registerHandler(
        "/rest/governance/proposals",
        [context](const drogon::HttpRequestPtr& req, Callback&& cb) {
            rest_governance_proposals(context, req, std::move(cb));
        },
        {drogon::Get});

    app.registerHandlerViaRegex(
        "/rest/governance/proposal/([a-fA-F0-9]{64})",
        [context](const drogon::HttpRequestPtr& req, Callback&& cb, const std::string& hash) {
            rest_governance_proposal(context, req, std::move(cb), hash);
        },
        {drogon::Get});
}
} // namespace rest

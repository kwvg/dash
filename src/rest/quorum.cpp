// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rest/quorum.h>

#include <chainparams.h>
#include <context.h>
#include <llmq/commitment.h>
#include <llmq/context.h>
#include <llmq/options.h>
#include <llmq/quorums.h>
#include <llmq/quorumsman.h>
#include <node/context.h>
#include <rest/common.h>
#include <sync.h>
#include <util/check.h>
#include <validation.h>

#include <drogon/HttpAppFramework.h>

#include <univalue.h>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

using node::NodeContext;
using rest::ParseAcceptFormat;
using rest::WriteJsonReply;

namespace {
void rest_quorum_list(const CoreContext& context,
                      const drogon::HttpRequestPtr& req,
                      rest::Callback&& cb)
{
    if (!rest::CheckWarmup(cb)) return;
    if (ParseAcceptFormat(req) != RESTResponseFormat::JSON) {
        rest::RESTERR(cb, drogon::k406NotAcceptable, "output format not found (available: json)");
        return;
    }

    ChainstateManager* chainman = rest::GetChainman(context, cb);
    if (!chainman) return;

    LLMQContext* llmq_ctx = rest::GetLLMQContext(context, cb);
    if (!llmq_ctx) return;

    const std::string height_str = rest::GetQueryParam(req, "height", "");

    const CBlockIndex* pblockindex{nullptr};
    {
        LOCK(cs_main);
        if (height_str.empty()) {
            pblockindex = chainman->ActiveChain().Tip();
        } else {
            int height{0};
            try {
                height = std::stoi(height_str);
            } catch (...) {
                rest::RESTERR(cb, drogon::k400BadRequest, "Invalid height: " + height_str);
                return;
            }
            if (height < 0 || height > chainman->ActiveChain().Height()) {
                rest::RESTERR(cb, drogon::k400BadRequest, "Block height out of range");
                return;
            }
            pblockindex = chainman->ActiveChain()[height];
        }
    }

    UniValue ret(UniValue::VOBJ);

    for (const auto& type : llmq::GetEnabledQuorumTypes(*chainman, pblockindex)) {
        const auto llmq_params_opt = Params().GetLLMQ(type);
        CHECK_NONFATAL(llmq_params_opt.has_value());
        const auto& llmq_params = llmq_params_opt.value();
        UniValue v(UniValue::VARR);

        auto quorums = llmq_ctx->qman->ScanQuorums(type, pblockindex, llmq_params.signingActiveQuorumCount);
        for (const auto& q : quorums) {
            size_t num_members = q->members.size();
            size_t num_valid_members = std::count_if(q->qc->validMembers.begin(),
                                                     q->qc->validMembers.begin() + num_members,
                                                     [](auto val) { return val; });
            double health_ratio = num_members > 0 ? double(num_valid_members) / double(num_members) : 0.0;
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << health_ratio;

            UniValue obj(UniValue::VOBJ);
            UniValue j(UniValue::VOBJ);
            if (llmq_params.useRotation) {
                j.pushKV("quorumIndex", q->qc->quorumIndex);
            }
            j.pushKV("creationHeight", q->m_quorum_base_block_index->nHeight);
            j.pushKV("minedBlockHash", q->minedBlockHash.ToString());
            j.pushKV("numValidMembers", static_cast<int>(num_valid_members));
            j.pushKV("healthRatio", ss.str());
            obj.pushKV(q->qc->quorumHash.ToString(), j);
            v.push_back(obj);
        }
        ret.pushKV(std::string(llmq_params.name), v);
    }

    WriteJsonReply(cb, ret);
}
} // anonymous namespace

namespace rest {
void RegisterQuorumHandlers(const CoreContext& context, drogon::HttpAppFramework& app)
{
    app.registerHandler(
        "/rest/quorum/list",
        [context](const drogon::HttpRequestPtr& req, Callback&& cb) {
            rest_quorum_list(context, req, std::move(cb));
        },
        {drogon::Get});
}
} // namespace rest

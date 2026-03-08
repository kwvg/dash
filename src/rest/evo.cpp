// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rest/evo.h>

#include <context.h>
#include <evo/deterministicmns.h>
#include <evo/smldiff.h>
#include <llmq/context.h>
#include <node/context.h>
#include <rest/common.h>
#include <sync.h>
#include <util/check.h>
#include <validation.h>

#include <drogon/HttpAppFramework.h>

#include <univalue.h>

#include <string>

using node::NodeContext;
using rest::ParseAcceptFormat;
using rest::WriteJsonReply;

namespace {
void rest_protx_diff(const CoreContext& context,
                     const drogon::HttpRequestPtr& req,
                     rest::Callback&& cb,
                     const std::string& base_height_str,
                     const std::string& block_height_str)
{
    if (!rest::CheckWarmup(cb)) return;
    if (ParseAcceptFormat(req) != RESTResponseFormat::JSON) {
        rest::RESTERR(cb, drogon::k406NotAcceptable, "output format not found (available: json)");
        return;
    }

    const NodeContext* node = rest::GetNodeContext(context, cb);
    if (!node) return;

    ChainstateManager* chainman = rest::GetChainman(context, cb);
    if (!chainman) return;

    LLMQContext* llmq_ctx = rest::GetLLMQContext(context, cb);
    if (!llmq_ctx) return;

    int base_height{0};
    int block_height{0};
    try {
        base_height = std::stoi(base_height_str);
        block_height = std::stoi(block_height_str);
    } catch (...) {
        rest::RESTERR(cb, drogon::k400BadRequest, "Invalid block height");
        return;
    }

    const bool extended = rest::GetQueryParam(req, "extended", "") == "true";

    uint256 baseBlockHash;
    uint256 blockHash;
    {
        LOCK(cs_main);
        const CChain& active_chain = chainman->ActiveChain();

        if (base_height < 0 || base_height > active_chain.Height() ||
            block_height < 0 || block_height > active_chain.Height()) {
            rest::RESTERR(cb, drogon::k400BadRequest, "Block height out of range");
            return;
        }

        baseBlockHash = *active_chain[base_height]->phashBlock;
        blockHash = *active_chain[block_height]->phashBlock;
    }

    CSimplifiedMNListDiff mnListDiff;
    std::string strError;
    {
        LOCK(cs_main);
        if (!BuildSimplifiedMNListDiff(*CHECK_NONFATAL(node->dmnman), *chainman,
                                       *llmq_ctx->quorum_block_processor, *llmq_ctx->qman,
                                       baseBlockHash, blockHash, mnListDiff, strError, extended))
        {
            rest::RESTERR(cb, drogon::k404NotFound, strError);
            return;
        }
    }

    WriteJsonReply(cb, mnListDiff.ToJson(extended));
}
} // anonymous namespace

namespace rest {
void RegisterEvoHandlers(const CoreContext& context, drogon::HttpAppFramework& app)
{
    app.registerHandlerViaRegex(
        "/rest/protx/diff/(\\d+)/(\\d+)",
        [context](const drogon::HttpRequestPtr& req, Callback&& cb,
                  const std::string& base_height, const std::string& block_height) {
            rest_protx_diff(context, req, std::move(cb), base_height, block_height);
        },
        {drogon::Get});
}
} // namespace rest

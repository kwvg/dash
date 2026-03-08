// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rest/chainlock.h>

#include <chainlock/chainlock.h>
#include <context.h>
#include <node/context.h>
#include <rest/common.h>
#include <streams.h>
#include <sync.h>
#include <util/check.h>
#include <validation.h>

#include <drogon/HttpAppFramework.h>

#include <univalue.h>

using node::NodeContext;
using rest::ParseAcceptFormat;
using rest::WriteJsonReply;

namespace {
void rest_chainlock(const CoreContext& context,
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

    const chainlock::ChainLockSig clsig = CHECK_NONFATAL(node->chainlocks)->GetBestChainLock();
    if (clsig.IsNull()) {
        rest::RESTERR(cb, drogon::k404NotFound, "No known ChainLock");
        return;
    }

    ChainstateManager* chainman = rest::GetChainman(context, cb);
    if (!chainman) return;

    UniValue result(UniValue::VOBJ);
    result.pushKV("blockhash", clsig.getBlockHash().GetHex());
    result.pushKV("height", clsig.getHeight());
    result.pushKV("signature", clsig.getSig().ToString());

    {
        LOCK(cs_main);
        result.pushKV("known_block", chainman->m_blockman.LookupBlockIndex(clsig.getBlockHash()) != nullptr);
    }

    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << clsig;
    result.pushKV("hex", HexStr(ssTx));

    WriteJsonReply(cb, result);
}
} // anonymous namespace

namespace rest {
void RegisterChainLockHandlers(const CoreContext& context, drogon::HttpAppFramework& app)
{
    app.registerHandler(
        "/rest/chainlock",
        [context](const drogon::HttpRequestPtr& req, Callback&& cb) {
            rest_chainlock(context, req, std::move(cb));
        },
        {drogon::Get});
}
} // namespace rest

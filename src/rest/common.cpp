// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rest/common.h>

#include <llmq/context.h>
#include <node/context.h>
#include <rpc/server.h>
#include <txmempool.h>
#include <util/system.h>
#include <validation.h>

using node::NodeContext;

static const struct {
    RESTResponseFormat rf;
    const char* name;
} rf_names[] = {
      {RESTResponseFormat::UNDEF, ""},
      {RESTResponseFormat::BINARY, "bin"},
      {RESTResponseFormat::HEX, "hex"},
      {RESTResponseFormat::JSON, "json"},
};

namespace rest {
drogon::HttpResponsePtr MakeResponse(drogon::HttpStatusCode code, drogon::ContentType ct, std::string body)
{
    auto resp = drogon::HttpResponse::newHttpResponse(code, ct);
    resp->setBody(std::move(body));
    return resp;
}

void WriteReply(const Callback& cb, drogon::ContentType ct, std::string body)
{
    cb(MakeResponse(drogon::k200OK, ct, std::move(body)));
}

bool RESTERR(const Callback& cb, drogon::HttpStatusCode status, std::string message)
{
    cb(MakeResponse(status, drogon::CT_TEXT_PLAIN, message + "\r\n"));
    return false;
}

std::string GetQueryParam(const drogon::HttpRequestPtr& req, const std::string& key, const std::string& default_value)
{
    const std::string& val = req->getParameter(key);
    return val.empty() ? default_value : val;
}

RESTResponseFormat ParseAcceptFormat(const drogon::HttpRequestPtr& req)
{
    const std::string& accept = req->getHeader("Accept");
    if (accept.empty() || accept.find("*/*") != std::string::npos ||
        accept.find("application/json") != std::string::npos) {
        return RESTResponseFormat::JSON;
    }
    if (accept.find("application/octet-stream") != std::string::npos) {
        return RESTResponseFormat::BINARY;
    }
    if (accept.find("text/plain") != std::string::npos) {
        return RESTResponseFormat::HEX;
    }
    return RESTResponseFormat::UNDEF;
}

RESTResponseFormat ParseDataFormat(std::string& param, const std::string& strReq)
{
    // Remove query string (if any, separated with '?') as it should not interfere with
    // parsing param and data format
    param = strReq.substr(0, strReq.rfind('?'));
    const std::string::size_type pos_format{param.rfind('.')};

    // No format string is found
    if (pos_format == std::string::npos) {
        return rf_names[0].rf;
    }

    // Match format string to available formats
    const std::string suffix(param, pos_format + 1);
    for (const auto& rf_name : rf_names) {
        if (suffix == rf_name.name) {
            param.erase(pos_format);
            return rf_name.rf;
        }
    }

    // If no suffix is found, return RESTResponseFormat::UNDEF and original string without query string
    return rf_names[0].rf;
}

std::string AvailableDataFormatsString()
{
    std::string formats;
    for (const auto& rf_name : rf_names) {
        if (strlen(rf_name.name) > 0) {
            formats.append(".");
            formats.append(rf_name.name);
            formats.append(", ");
        }
    }

    if (formats.length() > 0)
        return formats.substr(0, formats.length() - 2);

    return formats;
}

bool HasFormatSuffix(const std::string& path)
{
    const auto dot = path.rfind('.');
    if (dot == std::string::npos) return false;
    const std::string ext = path.substr(dot + 1);
    for (const auto& rf_name : rf_names) {
        if (strlen(rf_name.name) > 0 && ext == rf_name.name) return true;
    }
    return false;
}

bool CheckWarmup(const Callback& cb)
{
    std::string statusmessage;
    if (RPCIsInWarmup(&statusmessage))
         return RESTERR(cb, drogon::k503ServiceUnavailable, "Service temporarily unavailable: " + statusmessage);
    return true;
}

void AddDeprecationHeaders(const drogon::HttpResponsePtr& resp, const std::string& link_target)
{
    resp->addHeader("Deprecation", "true");
    resp->addHeader("Link", "<" + link_target + ">; rel=\"successor-version\"");
}

NodeContext* GetNodeContext(const CoreContext& context, const Callback& cb)
{
    auto* node_context = GetContext<NodeContext>(context);
    if (!node_context) {
        RESTERR(cb, drogon::k500InternalServerError,
                strprintf("%s:%d (%s)\n"
                          "Internal bug detected: Node context not found!\n"
                          "You may report this issue here: %s\n",
                          __FILE__, __LINE__, __func__, PACKAGE_BUGREPORT));
        return nullptr;
    }
    return node_context;
}

CTxMemPool* GetMemPool(const CoreContext& context, const Callback& cb)
{
    auto* node_context = GetContext<NodeContext>(context);
    if (!node_context || !node_context->mempool) {
        RESTERR(cb, drogon::k404NotFound, "Mempool disabled or instance not found");
        return nullptr;
    }
    return node_context->mempool.get();
}

ChainstateManager* GetChainman(const CoreContext& context, const Callback& cb)
{
    auto node_context = GetContext<NodeContext>(context);
    if (!node_context || !node_context->chainman) {
        RESTERR(cb, drogon::k500InternalServerError,
                strprintf("%s:%d (%s)\n"
                          "Internal bug detected: Chainman disabled or instance not found!\n"
                          "You may report this issue here: %s\n",
                          __FILE__, __LINE__, __func__, PACKAGE_BUGREPORT));
        return nullptr;
    }
    return node_context->chainman.get();
}

LLMQContext* GetLLMQContext(const CoreContext& context, const Callback& cb)
{
    auto node_context = GetContext<NodeContext>(context);
    if (!node_context || !node_context->llmq_ctx) {
        RESTERR(cb, drogon::k500InternalServerError,
                strprintf("%s:%d (%s)\n"
                          "Internal bug detected: LLMQ context not found!\n"
                          "You may report this issue here: %s\n",
                          __FILE__, __LINE__, __func__, PACKAGE_BUGREPORT));
        return nullptr;
    }
    return node_context->llmq_ctx.get();
}
} // namespace rest

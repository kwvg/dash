// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_REST_COMMON_H
#define BITCOIN_REST_COMMON_H

#include <context.h>

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

#include <functional>
#include <string>

class ChainstateManager;
class CTxMemPool;
struct LLMQContext;
namespace node {
struct NodeContext;
} // namespace node

enum class RESTResponseFormat {
    UNDEF,
    BINARY,
    HEX,
    JSON,
};

namespace rest {
/** REST handler completion callback. */
using Callback = std::function<void(const drogon::HttpResponsePtr&)>;

/**
 * Build an HTTP response with the given status code, content type, and body.
 *
 * @param[in] code  HTTP status code.
 * @param[in] ct    Content type of the response body.
 * @param[in] body  Response body text.
 * @returns         A fully-formed HTTP response.
 */
drogon::HttpResponsePtr MakeResponse(drogon::HttpStatusCode code, drogon::ContentType ct, std::string body);

/**
 * Send a 200 OK response with the given content type and body.
 *
 * @param[in] cb    Callback to invoke with the response.
 * @param[in] ct    Content type of the response body.
 * @param[in] body  Response body text.
 */
void WriteReply(const Callback& cb, drogon::ContentType ct, std::string body);

/**
 * Send an error response and return false (for early-return idiom).
 *
 * @param[in] cb       Callback to invoke with the error response.
 * @param[in] status   HTTP status code for the error.
 * @param[in] message  Human-readable error message.
 * @returns            Always false, for use in `return RESTERR(...)`.
 */
bool RESTERR(const Callback& cb, drogon::HttpStatusCode status, std::string message);

/**
 * Extract a query-string parameter, returning a default if absent or empty.
 *
 * @param[in] req            The incoming HTTP request.
 * @param[in] key            Query parameter name.
 * @param[in] default_value  Value returned when the parameter is missing or empty.
 * @returns                  The parameter value, or @p default_value.
 */
std::string GetQueryParam(const drogon::HttpRequestPtr& req, const std::string& key, const std::string& default_value);

/**
 * Check whether the RPC server is still warming up.
 *
 * If it is, an HTTP 503 error is sent via @p cb and false is returned.
 *
 * @param[in] cb  Callback to invoke with an error response on warmup.
 * @returns       true if the server is ready, false if still warming up.
 */
bool CheckWarmup(const Callback& cb);

/**
 * Annotate an HTTP response with RFC 8594 / draft-ietf-httpapi-deprecation
 * headers indicating the endpoint is deprecated.
 *
 * @param[in] resp           The response to annotate.
 * @param[in] link_target    The preferred replacement URI template (used in the Link header).
 */
void AddDeprecationHeaders(const drogon::HttpResponsePtr& resp, const std::string& link_target);

/**
 * Get the node context.
 *
 * @param[in]  context  The CoreContext to extract NodeContext from.
 * @param[in]  cb       Callback invoked with an error response if NodeContext is not found.
 * @returns             Pointer to NodeContext or nullptr if not found.
 */
node::NodeContext* GetNodeContext(const CoreContext& context, const Callback& cb);

/**
 * Get the node context mempool.
 *
 * @param[in]  context  The core context to extract the mempool from.
 * @param[in]  cb       Callback invoked with an error response if mempool is not found.
 * @returns             Pointer to the mempool or nullptr if no mempool found.
 */
CTxMemPool* GetMemPool(const CoreContext& context, const Callback& cb);

/**
 * Get the node context chainstatemanager.
 *
 * @param[in]  context  The core context to extract ChainstateManager from.
 * @param[in]  cb       Callback invoked with an error response if ChainstateManager is not found.
 * @returns             Pointer to ChainstateManager or nullptr if none found.
 */
ChainstateManager* GetChainman(const CoreContext& context, const Callback& cb);

/**
 * Get the node context LLMQContext.
 *
 * @param[in]  context  The core context to extract LLMQContext from.
 * @param[in]  cb       Callback invoked with an error response if LLMQContext is not found.
 * @returns             Pointer to LLMQContext or nullptr if none found.
 */
LLMQContext* GetLLMQContext(const CoreContext& context, const Callback& cb);

/**
 * Determine response format from the Accept header.
 *
 * @param[in] req  The incoming HTTP request.
 * @returns        The negotiated response format.
 */
RESTResponseFormat ParseAcceptFormat(const drogon::HttpRequestPtr& req);

/**
 * Parse a URI to get the data format and URI without data format
 * and query string.
 *
 * @param[out]  param   The strReq without the data format string and
 *                      without the query string (if any).
 * @param[in]   strReq  The URI to be parsed.
 * @return      RESTResponseFormat that was parsed from the URI.
 */
RESTResponseFormat ParseDataFormat(std::string& param, const std::string& strReq);

/**
 * Return a comma-separated list of recognised format suffixes.
 */
std::string AvailableDataFormatsString();

/**
 * Return true when @p path ends with a recognised format suffix.
 */
bool HasFormatSuffix(const std::string& path);
} // namespace rest

#endif // BITCOIN_REST_COMMON_H

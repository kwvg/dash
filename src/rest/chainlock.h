// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_REST_CHAINLOCK_H
#define BITCOIN_REST_CHAINLOCK_H

#include <context.h>

namespace drogon {
class HttpAppFramework;
} // namespace drogon

namespace rest {
void RegisterChainLockHandlers(const CoreContext& context, drogon::HttpAppFramework& app);
} // namespace rest

#endif // BITCOIN_REST_CHAINLOCK_H

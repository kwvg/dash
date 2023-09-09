// Copyright (c) 2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coinjoin/context.h>

#include <net.h>
#include <policy/fees.h>
#include <txmempool.h>

#ifdef ENABLE_WALLET
#include <coinjoin/client.h>
#endif // ENABLE_WALLET
#include <coinjoin/server.h>

CJContext::CJContext(CChainState& chainstate, CConnman& connman, CTxMemPool& mempool, const CMasternodeSync& mn_sync, bool relay_txes) :
#ifdef ENABLE_WALLET
    clientman {
        [&]() -> CJClientManager* const {
            assert(::coinJoinClientManagers == nullptr);
            ::coinJoinClientManagers = std::make_unique<CJClientManager>(connman, mempool, mn_sync);
            return ::coinJoinClientManagers.get();
        }()
    },
    queueman {
        [&]() -> CCoinJoinClientQueueManager* const {
            if (relay_txes) {
                assert(::coinJoinClientQueueManager == nullptr);
                ::coinJoinClientQueueManager = std::make_unique<CCoinJoinClientQueueManager>(connman, mn_sync);
                return ::coinJoinClientQueueManager.get();
            }
            return nullptr;
        }()
    },
#endif // ENABLE_WALLET
    server {
        [&]() -> CCoinJoinServer* const {
            assert(::coinJoinServer == nullptr);
            ::coinJoinServer = std::make_unique<CCoinJoinServer>(chainstate, connman, mempool, mn_sync);
            return ::coinJoinServer.get();
        }()
    }
{}

CJContext::~CJContext() {
#ifdef ENABLE_WALLET
    ::coinJoinClientQueueManager.reset();
    ::coinJoinClientManagers.reset();
#endif // ENABLE_WALLET
    ::coinJoinServer.reset();
}

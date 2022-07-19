// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/context.h>

#include <banman.h>
#include <interfaces/chain.h>
#include <net.h>
#include <net_processing.h>
#include <scheduler.h>

#include <llmq/debug.h>
#include <llmq/blockprocessor.h>
#include <llmq/commitment.h>

NodeContext::NodeContext() {}
NodeContext::~NodeContext() {}

llmq::Context::Context(NodeContext& node, CEvoDB& evoDb, CTxMemPool& mempool, CConnman& connman, bool unitTests, bool fWipe) {
    blsWorker = std::make_shared<CBLSWorker>();
    chainLocksHandler = std::make_unique<CChainLocksHandler>(mempool, connman, node);
    quorumDKGDebugManager = std::make_unique<CDKGDebugManager>();
    quorumBlockProcessor = std::make_unique<CQuorumBlockProcessor>(evoDb, connman);
    quorumDKGSessionManager = std::make_unique<CDKGSessionManager>(connman, blsWorker, *quorumDKGDebugManager, unitTests, fWipe);
    quorumManager = std::make_unique<CQuorumManager>(evoDb, connman, blsWorker, *quorumBlockProcessor, *quorumDKGSessionManager);
    quorumSigSharesManager = std::make_unique<CSigSharesManager>(connman, node);
    quorumSigningManager = std::make_unique<CSigningManager>(connman, *quorumManager, *quorumSigSharesManager, unitTests, fWipe);
    quorumInstantSendManager = std::make_unique<CInstantSendManager>(mempool, connman, node, unitTests, fWipe);

    // NOTE: we use this only to wipe the old db, do NOT use it for anything else
    // TODO: remove it in some future version
    auto llmqDbTmp = std::make_unique<CDBWrapper>(unitTests ? "" : (GetDataDir() / "llmq"), 1 << 20, unitTests, true);
}

llmq::Context::~Context() {
    llmq::DestroyLLMQSystem();
}

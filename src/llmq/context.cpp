// Copyright (c) 2018-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/context.h>

#include <dbwrapper.h>
#include <validation.h>

#include <llmq/blockprocessor.h>
#include <llmq/chainlocks.h>
#include <llmq/commitment.h>
#include <llmq/debug.h>
#include <llmq/dkgsessionmgr.h>
#include <llmq/ehf_signals.h>
#include <llmq/instantsend.h>
#include <llmq/quorums.h>
#include <llmq/signing.h>
#include <llmq/signing_shares.h>

LLMQContext::LLMQContext(ChainstateManager& chainman, CDeterministicMNManager& dmnman, CEvoDB& evo_db,
                         CMasternodeMetaMan& mn_metaman, CMNHFManager& mnhfman, CSporkManager& sporkman,
                         CTxMemPool& mempool, const CActiveMasternodeManager* const mn_activeman,
                         const CMasternodeSync& mn_sync, bool unit_tests, bool wipe) :
    is_masternode{mn_activeman != nullptr},
    bls_worker{std::make_shared<CBLSWorker>()},
    dkg_debugman{std::make_unique<llmq::CDKGDebugManager>()},
    quorum_block_processor{std::make_unique<llmq::CQuorumBlockProcessor>(chainman.ActiveChainstate(), dmnman, evo_db)},
    qdkgsman{std::make_unique<llmq::CDKGSessionManager>(*bls_worker, chainman.ActiveChainstate(), dmnman, *dkg_debugman,
                                                        mn_metaman, *quorum_block_processor, mn_activeman, sporkman,
                                                        unit_tests, wipe)},
    qman{std::make_unique<llmq::CQuorumManager>(*bls_worker, chainman.ActiveChainstate(), dmnman, *qdkgsman, evo_db,
                                                *quorum_block_processor, mn_activeman, mn_sync, sporkman, unit_tests,
                                                wipe)},
    sigman{std::make_unique<llmq::CSigningManager>(mn_activeman, chainman.ActiveChainstate(), *qman, unit_tests, wipe)},
    shareman{std::make_unique<llmq::CSigSharesManager>(*sigman, mn_activeman, *qman, sporkman)},
    clhandler{std::make_unique<llmq::CChainLocksHandler>(chainman.ActiveChainstate(), *qman, *sigman, *shareman, sporkman,
                                                         mempool, mn_sync, is_masternode)},
    isman{std::make_unique<llmq::CInstantSendManager>(*clhandler, chainman.ActiveChainstate(), *qman, *sigman, *shareman,
                                                      sporkman, mempool, mn_sync, is_masternode, unit_tests, wipe)},
    ehfSignalsHandler{std::make_unique<llmq::CEHFSignalsHandler>(chainman, mnhfman, *sigman, *shareman, *qman)}
{
    // Have to start it early to let VerifyDB check ChainLock signatures in coinbase
    bls_worker->Start();
}

LLMQContext::~LLMQContext() {
    bls_worker->Stop();
}

void LLMQContext::Interrupt() {
    shareman->InterruptWorkerThread();
    sigman->InterruptWorkerThread();
    isman->InterruptWorkerThread();
}

void LLMQContext::Start(CConnman& connman, PeerManager& peerman)
{
    if (is_masternode) {
        qdkgsman->StartThreads(connman, peerman);
    }
    qman->Start();
    shareman->RegisterAsRecoveredSigsListener();
    shareman->StartWorkerThread(connman, peerman);
    sigman->StartWorkerThread(peerman);
    clhandler->Start(*isman);
    isman->Start(peerman);
}

void LLMQContext::Stop() {
    isman->Stop();
    clhandler->Stop();
    sigman->StopWorkerThread();
    shareman->StopWorkerThread();
    shareman->UnregisterAsRecoveredSigsListener();
    qman->Stop();
    if (is_masternode) {
        qdkgsman->StopThreads();
    }
}

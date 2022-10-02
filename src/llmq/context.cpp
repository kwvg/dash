// Copyright (c) 2018-2022 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/context.h>

#include <consensus/validation.h>
#include <dbwrapper.h>

#include <llmq/blockprocessor.h>
#include <llmq/chainlocks.h>
#include <llmq/commitment.h>
#include <llmq/debug.h>
#include <llmq/dkgsessionmgr.h>
#include <llmq/instantsend.h>
#include <llmq/quorums.h>
#include <llmq/signing.h>
#include <llmq/signing_shares.h>
#include <llmq/utils.h>

LLMQContext::LLMQContext(CEvoDB& evoDb, CTxMemPool& mempool, CConnman& connman, CSporkManager& sporkManager, bool unitTests, bool fWipe) {
    Create(evoDb, mempool, connman, sporkManager, unitTests, fWipe);

    /* Context aliases to globals used by the LLMQ system */
    quorum_block_processor = llmq::quorumBlockProcessor.get();
    qman = llmq::quorumManager.get();
    sigman = llmq::quorumSigningManager.get();
    shareman = llmq::quorumSigSharesManager.get();
    clhandler = llmq::chainLocksHandler.get();
    isman = llmq::quorumInstantSendManager.get();
}

LLMQContext::~LLMQContext() {
    isman = nullptr;
    clhandler = nullptr;
    shareman = nullptr;
    sigman = nullptr;
    qman = nullptr;
    quorum_block_processor = nullptr;

    Destroy();
}

void LLMQContext::Create(CEvoDB& evoDb, CTxMemPool& mempool, CConnman& connman, CSporkManager& sporkManager, bool unitTests, bool fWipe) {
    bls_worker = std::make_shared<CBLSWorker>();

    dkg_debugman = std::make_unique<llmq::CDKGDebugManager>();
    llmq::quorumBlockProcessor = std::make_unique<llmq::CQuorumBlockProcessor>(evoDb, connman);
    qdkgsman = std::make_unique<llmq::CDKGSessionManager>(connman, *bls_worker, *dkg_debugman, *llmq::quorumBlockProcessor, sporkManager, unitTests, fWipe);
    llmq::quorumManager = std::make_unique<llmq::CQuorumManager>(evoDb, connman, *bls_worker, *llmq::quorumBlockProcessor, *qdkgsman);
    llmq::quorumSigningManager = std::make_unique<llmq::CSigningManager>(connman, *llmq::quorumManager, unitTests, fWipe);
    llmq::quorumSigSharesManager = std::make_unique<llmq::CSigSharesManager>(connman, *llmq::quorumManager, *llmq::quorumSigningManager);
    llmq::chainLocksHandler = std::make_unique<llmq::CChainLocksHandler>(mempool, connman, sporkManager, *llmq::quorumSigningManager, *llmq::quorumSigSharesManager);
    llmq::quorumInstantSendManager = std::make_unique<llmq::CInstantSendManager>(mempool, connman, sporkManager, *llmq::quorumManager, *llmq::quorumSigningManager, *llmq::quorumSigSharesManager, *llmq::chainLocksHandler, unitTests, fWipe);

    // NOTE: we use this only to wipe the old db, do NOT use it for anything else
    // TODO: remove it in some future version
    auto llmqDbTmp = std::make_unique<CDBWrapper>(unitTests ? "" : (GetDataDir() / "llmq"), 1 << 20, unitTests, true);
}

void LLMQContext::Destroy() {
    llmq::quorumInstantSendManager.reset();
    llmq::chainLocksHandler.reset();
    llmq::quorumSigSharesManager.reset();
    llmq::quorumSigningManager.reset();
    llmq::quorumManager.reset();
    qdkgsman.reset();
    llmq::quorumBlockProcessor.reset();
    dkg_debugman.reset();
    bls_worker.reset();
    {
        LOCK(llmq::cs_llmq_vbc);
        llmq::llmq_versionbitscache.Clear();
    }
}

void LLMQContext::Interrupt() {
    if (llmq::quorumSigSharesManager != nullptr) {
        llmq::quorumSigSharesManager->InterruptWorkerThread();
    }
    if (llmq::quorumInstantSendManager != nullptr) {
        llmq::quorumInstantSendManager->InterruptWorkerThread();
    }
}

void LLMQContext::Start() {
    if (bls_worker != nullptr) {
        bls_worker->Start();
    }
    if (qdkgsman != nullptr) {
        qdkgsman->StartThreads();
    }
    if (llmq::quorumManager != nullptr) {
        llmq::quorumManager->Start();
    }
    if (llmq::quorumSigSharesManager != nullptr) {
        llmq::quorumSigSharesManager->RegisterAsRecoveredSigsListener();
        llmq::quorumSigSharesManager->StartWorkerThread();
    }
    if (llmq::chainLocksHandler != nullptr) {
        llmq::chainLocksHandler->Start();
    }
    if (llmq::quorumInstantSendManager != nullptr) {
        llmq::quorumInstantSendManager->Start();
    }
}

void LLMQContext::Stop() {
    if (llmq::quorumInstantSendManager != nullptr) {
        llmq::quorumInstantSendManager->Stop();
    }
    if (llmq::chainLocksHandler != nullptr) {
        llmq::chainLocksHandler->Stop();
    }
    if (llmq::quorumSigSharesManager != nullptr) {
        llmq::quorumSigSharesManager->StopWorkerThread();
        llmq::quorumSigSharesManager->UnregisterAsRecoveredSigsListener();
    }
    if (llmq::quorumManager != nullptr) {
        llmq::quorumManager->Stop();
    }
    if (qdkgsman != nullptr) {
        qdkgsman->StopThreads();
    }
    if (bls_worker != nullptr) {
        bls_worker->Stop();
    }
}

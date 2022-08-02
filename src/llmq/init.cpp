// Copyright (c) 2018-2022 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/init.h>

#include <llmq/quorums.h>
#include <llmq/blockprocessor.h>
#include <llmq/commitment.h>
#include <llmq/chainlocks.h>
#include <llmq/debug.h>
#include <llmq/dkgsessionmgr.h>
#include <llmq/instantsend.h>
#include <llmq/signing.h>
#include <llmq/signing_shares.h>
#include <llmq/utils.h>
#include <consensus/validation.h>

#include <dbwrapper.h>

namespace llmq
{

CBLSWorker* blsWorker;

void InitLLMQSystem(CEvoDB& evoDb, CTxMemPool& mempool, CConnman& connman, CSporkManager& sporkManager, bool unitTests, bool fWipe)
{
    blsWorker = new CBLSWorker();

    quorumDKGDebugManager = new CDKGDebugManager();
    quorumBlockProcessor = new CQuorumBlockProcessor(evoDb, connman);
    quorumDKGSessionManager = new CDKGSessionManager(connman, *blsWorker, sporkManager, unitTests, fWipe);
    quorumManager = new CQuorumManager(evoDb, connman, *blsWorker, *quorumDKGSessionManager);
    quorumSigSharesManager = new CSigSharesManager(connman);
    quorumSigningManager = new CSigningManager(connman, unitTests, fWipe);
    chainLocksHandler = new CChainLocksHandler(mempool, connman, sporkManager);
    quorumInstantSendManager = new CInstantSendManager(mempool, connman, sporkManager, unitTests, fWipe);

    // NOTE: we use this only to wipe the old db, do NOT use it for anything else
    // TODO: remove it in some future version
    auto llmqDbTmp = std::make_unique<CDBWrapper>(unitTests ? "" : (GetDataDir() / "llmq"), 1 << 20, unitTests, true);
}

void DestroyLLMQSystem()
{
    delete quorumInstantSendManager;
    quorumInstantSendManager = nullptr;
    delete chainLocksHandler;
    chainLocksHandler = nullptr;
    delete quorumSigningManager;
    quorumSigningManager = nullptr;
    delete quorumSigSharesManager;
    quorumSigSharesManager = nullptr;
    delete quorumManager;
    quorumManager = nullptr;
    delete quorumDKGSessionManager;
    quorumDKGSessionManager = nullptr;
    delete quorumBlockProcessor;
    quorumBlockProcessor = nullptr;
    delete quorumDKGDebugManager;
    quorumDKGDebugManager = nullptr;
    delete blsWorker;
    blsWorker = nullptr;
    LOCK(cs_llmq_vbc);
    llmq_versionbitscache.Clear();
}

void StartLLMQSystem()
{
    if (blsWorker != nullptr) {
        blsWorker->Start();
    }
    if (quorumDKGSessionManager != nullptr) {
        quorumDKGSessionManager->StartThreads();
    }
    if (quorumManager != nullptr) {
        quorumManager->Start();
    }
    if (quorumSigSharesManager != nullptr) {
        quorumSigSharesManager->RegisterAsRecoveredSigsListener();
        quorumSigSharesManager->StartWorkerThread();
    }
    if (chainLocksHandler != nullptr) {
        chainLocksHandler->Start();
    }
    if (quorumInstantSendManager != nullptr) {
        quorumInstantSendManager->Start();
    }
}

void StopLLMQSystem()
{
    if (quorumInstantSendManager != nullptr) {
        quorumInstantSendManager->Stop();
    }
    if (chainLocksHandler != nullptr) {
        chainLocksHandler->Stop();
    }
    if (quorumSigSharesManager != nullptr) {
        quorumSigSharesManager->StopWorkerThread();
        quorumSigSharesManager->UnregisterAsRecoveredSigsListener();
    }
    if (quorumManager != nullptr) {
        quorumManager->Stop();
    }
    if (quorumDKGSessionManager != nullptr) {
        quorumDKGSessionManager->StopThreads();
    }
    if (blsWorker != nullptr) {
        blsWorker->Stop();
    }
}

void InterruptLLMQSystem()
{
    if (quorumSigSharesManager != nullptr) {
        quorumSigSharesManager->InterruptWorkerThread();
    }
    if (quorumInstantSendManager != nullptr) {
        quorumInstantSendManager->InterruptWorkerThread();
    }
}

} // namespace llmq

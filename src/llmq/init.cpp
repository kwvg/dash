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

#include <node/context.h>

#include <dbwrapper.h>

namespace llmq
{
void DestroyLLMQSystem() {
    LOCK(cs_llmq_vbc);
    llmq_versionbitscache.Clear();
}

void StartLLMQSystem(Context &ctx) {
    if (ctx.blsWorker) {
        ctx.blsWorker->Start();
    }
    if (ctx.quorumDKGSessionManager) {
        ctx.quorumDKGSessionManager->StartThreads();
    }
    if (ctx.quorumManager) {
        ctx.quorumManager->Start();
    }
    if (ctx.quorumSigSharesManager != nullptr && ctx.quorumSigningManager != nullptr) {
        ctx.quorumSigningManager->RegisterRecoveredSigsListener(ctx.quorumSigSharesManager.get());
        ctx.quorumSigSharesManager->StartWorkerThread();
    }
    if (ctx.chainLocksHandler) {
        ctx.chainLocksHandler->Start();
    }
    if (ctx.quorumInstantSendManager) {
        ctx.quorumInstantSendManager->Start();
    }
}

void StopLLMQSystem(Context &ctx) {
    if (ctx.quorumInstantSendManager) {
        ctx.quorumInstantSendManager->Stop();
    }
    if (ctx.chainLocksHandler) {
        ctx.chainLocksHandler->Stop();
    }
    if (ctx.quorumSigSharesManager != nullptr && ctx.quorumSigningManager != nullptr) {
        ctx.quorumSigSharesManager->StopWorkerThread();
        ctx.quorumSigningManager->UnregisterRecoveredSigsListener(ctx.quorumSigSharesManager.get());
    }
    if (ctx.quorumManager) {
        ctx.quorumManager->Stop();
    }
    if (ctx.quorumDKGSessionManager) {
        ctx.quorumDKGSessionManager->StopThreads();
    }
    if (ctx.blsWorker) {
        ctx.blsWorker->Stop();
    }
}

void InterruptLLMQSystem(Context &ctx) {
    if (ctx.quorumSigSharesManager) {
        ctx.quorumSigSharesManager->InterruptWorkerThread();
    }
    if (ctx.quorumInstantSendManager) {
        ctx.quorumInstantSendManager->InterruptWorkerThread();
    }
}

} // namespace llmq

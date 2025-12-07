// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/context.h>

#include <bls/bls_worker.h>
#include <chainlock/chainlock.h>
#include <instantsend/instantsend.h>
#include <llmq/blockprocessor.h>
#include <llmq/quorums.h>
#include <llmq/signing.h>
#include <llmq/snapshot.h>
#include <validation.h>

LLMQContext::LLMQContext(CChainState& active_chainstate, CDeterministicMNManager& dmnman, CEvoDB& evo_db,
                         CSporkManager& sporkman, CTxMemPool& mempool, const CMasternodeSync& mn_sync,
                         const util::DbWrapperParams& db_params) :
    bls_worker{std::make_shared<CBLSWorker>()},
    qsnapman{std::make_unique<llmq::CQuorumSnapshotManager>(evo_db)},
    quorum_block_processor{
        std::make_unique<llmq::CQuorumBlockProcessor>(active_chainstate, dmnman, evo_db, *qsnapman)},
    qman{std::make_unique<llmq::CQuorumManager>(*bls_worker, active_chainstate, dmnman,
                                                *quorum_block_processor, *qsnapman, db_params)},
    sigman{std::make_unique<llmq::CSigningManager>(*qman, db_params)},
    clhandler{std::make_unique<llmq::CChainLocksHandler>(active_chainstate, *qman, sporkman, mempool, mn_sync)},
    isman{std::make_unique<llmq::CInstantSendManager>(*clhandler, active_chainstate, *sigman, sporkman,
                                                      mempool, mn_sync, db_params)}
{
    // Have to start it early to let VerifyDB check ChainLock signatures in coinbase
    bls_worker->Start();
}

LLMQContext::~LLMQContext()
{
    bls_worker->Stop();
}

void LLMQContext::Start()
{
    qman->Start();
    clhandler->Start(*isman);
}

void LLMQContext::Stop()
{
    clhandler->Stop();
    qman->Stop();
}

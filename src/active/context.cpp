// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <active/context.h>

#include <active/dkgsessionhandler.h>
#include <active/quorums.h>
#include <active/masternode.h>
#include <chainlock/chainlock.h>
#include <chainlock/signing.h>
#include <coinjoin/server.h>
#include <governance/governance.h>
#include <governance/signing.h>
#include <instantsend/instantsend.h>
#include <instantsend/signing.h>
#include <llmq/context.h>
#include <llmq/debug.h>
#include <llmq/dkgsessionmgr.h>
#include <llmq/ehf_signals.h>
#include <llmq/quorums.h>
#include <llmq/observer/quorums.h>
#include <llmq/signing_shares.h>
#include <validation.h>

ActiveContext::ActiveContext(CBLSWorker& bls_worker, ChainstateManager& chainman, CConnman& connman, CDeterministicMNManager& dmnman,
                             CDSTXManager& dstxman, CGovernanceManager& govman, CMasternodeMetaMan& mn_metaman,
                             CMNHFManager& mnhfman, CSporkManager& sporkman, CTxMemPool& mempool, llmq::CChainLocksHandler& clhandler,
                             llmq::CInstantSendManager& isman, llmq::CQuorumBlockProcessor& qblockman, llmq::CQuorumManager& qman,
                             llmq::CQuorumSnapshotManager& qsnapman, llmq::CSigningManager& sigman, PeerManager& peerman,
                             const CChainParams& chainparams, const CMasternodeSync& mn_sync, const CBLSSecretKey& operator_sk,
                             const util::DbWrapperParams& db_params, bool quorums_recovery, bool quorums_watch) :
    m_clhandler{clhandler},
    m_isman{isman},
    m_qman{qman},
    dkgdbgman{std::make_unique<llmq::CDKGDebugManager>()},
    nodeman{std::make_unique<CActiveMasternodeManager>(connman, dmnman, chainparams, operator_sk)},
    cj_server{std::make_unique<CCoinJoinServer>(chainman, connman, dmnman, dstxman, mn_metaman, mempool, peerman, *nodeman, mn_sync, isman)},
    qdkgsman{std::make_unique<llmq::CDKGSessionManager>(chainman.ActiveChainstate(), dmnman, qsnapman, chainparams, sporkman, db_params,
                                                        quorums_watch)},
    shareman{std::make_unique<llmq::CSigSharesManager>(connman, chainman.ActiveChainstate(), sigman, peerman, *nodeman, chainparams,
                                                       qman, sporkman)},
    ehf_sighandler{
        std::make_unique<llmq::CEHFSignalsHandler>(chainman, mnhfman, sigman, *shareman, chainparams, qman)},
    cl_signer{std::make_unique<chainlock::ChainLockSigner>(chainman.ActiveChainstate(), clhandler, sigman, *shareman, sporkman, mn_sync)},
    gov_signer{std::make_unique<GovernanceSigner>(connman, dmnman, govman, *nodeman, chainman, mn_sync)},
    is_signer{std::make_unique<instantsend::InstantSendSigner>(chainman.ActiveChainstate(), clhandler, isman, sigman, *shareman, qman,
                                                               sporkman, mempool, mn_sync)},
    qman_handler{std::make_unique<llmq::QuorumParticipant>(bls_worker, dmnman, qman, qsnapman, *nodeman, chainparams, mn_sync, sporkman,
                                                           quorums_recovery, quorums_watch)}
{
    qdkgsman->InitializeHandlers(
        [&](const Consensus::LLMQParams& llmq_params, int quorum_idx) -> std::unique_ptr<llmq::CDKGSessionHandler> {
            return std::make_unique<llmq::dkg::ActiveSessionHandler>(bls_worker, chainman.ActiveChainstate(), dmnman, mn_metaman,
                                                                     *dkgdbgman, *qdkgsman, qblockman, qsnapman, *nodeman, chainparams,
                                                                     sporkman, llmq_params, quorums_watch, quorum_idx);
        });
    m_clhandler.ConnectSigner(cl_signer.get());
    m_isman.ConnectSigner(is_signer.get());
    m_qman.ConnectManagers(qman_handler.get(), qdkgsman.get());
}

ActiveContext::~ActiveContext()
{
    m_qman.DisconnectManagers();
    m_isman.DisconnectSigner();
    m_clhandler.DisconnectSigner();
}

void ActiveContext::Interrupt()
{
    shareman->InterruptWorkerThread();
}

void ActiveContext::Start(CConnman& connman, PeerManager& peerman)
{
    qdkgsman->StartThreads(connman, peerman);
    shareman->StartWorkerThread();
    cl_signer->RegisterAsRecoveredSigsListener();
    is_signer->RegisterAsRecoveredSigsListener();
    shareman->RegisterAsRecoveredSigsListener();
}

void ActiveContext::Stop()
{
    shareman->UnregisterAsRecoveredSigsListener();
    is_signer->UnregisterAsRecoveredSigsListener();
    cl_signer->UnregisterAsRecoveredSigsListener();
    shareman->StopWorkerThread();
    qdkgsman->StopThreads();
}

void ActiveContext::UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload)
{
    if (fInitialDownload || pindexNew == pindexFork) // In IBD or blocks were disconnected without any new ones
        return;

    nodeman->UpdatedBlockTip(pindexNew, pindexFork, fInitialDownload);
    ehf_sighandler->UpdatedBlockTip(pindexNew);
    gov_signer->UpdatedBlockTip(pindexNew);
    qdkgsman->UpdatedBlockTip(pindexNew, fInitialDownload);
}

void ActiveContext::NotifyRecoveredSig(const std::shared_ptr<const llmq::CRecoveredSig>& sig, bool proactive_relay)
{
    shareman->NotifyRecoveredSig(sig, proactive_relay);
}

// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/observer/context.h>

#include <llmq/debug.h>
#include <llmq/dkgsessionmgr.h>
#include <llmq/observer/quorums.h>
#include <llmq/quorums.h>

namespace llmq {
ObserverContext::ObserverContext(CBLSWorker& bls_worker, CChainState& chainstate, CDeterministicMNManager& dmnman,
                                 CMasternodeMetaMan& mn_metaman, CMasternodeSync& mn_sync, llmq::CQuorumBlockProcessor& qblockman,
                                 llmq::CQuorumManager& qman, llmq::CQuorumSnapshotManager& qsnapman, const CSporkManager& sporkman,
                                 const util::DbWrapperParams& db_params, bool quorums_recovery) :
    m_qman{qman},
    dkgdbgman{std::make_unique<llmq::CDKGDebugManager>()},
    qdkgsman{std::make_unique<llmq::CDKGSessionManager>(chainstate, dmnman, qsnapman, sporkman, db_params, /*quorums_watch=*/true)},
    qman_handler{std::make_unique<llmq::QuorumObserver>(dmnman, qman, qsnapman, mn_sync, sporkman, quorums_recovery)}
{
    qdkgsman->InitializeHandlers(
        [&](const Consensus::LLMQParams& llmq_params, int quorum_idx) -> std::unique_ptr<llmq::CDKGSessionHandler> {
            return std::make_unique<llmq::CDKGSessionHandler>(bls_worker, dmnman, *dkgdbgman, *qdkgsman, qsnapman, llmq_params,
                                                              /*quorums_watch=*/true, quorum_idx);
        });
    m_qman.ConnectManagers(qman_handler.get(), qdkgsman.get());
}

ObserverContext::~ObserverContext()
{
    m_qman.DisconnectManagers();
}
} // namespace llmq

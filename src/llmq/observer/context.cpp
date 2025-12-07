// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/observer/context.h>

#include <llmq/dkgsessionmgr.h>
#include <llmq/observer/quorums.h>
#include <llmq/quorums.h>

namespace llmq {
ObserverContext::ObserverContext(CBLSWorker& bls_worker, CChainState& chainstate, CDeterministicMNManager& dmnman,
                                 CMasternodeMetaMan& mn_metaman, CMasternodeSync& mn_sync, llmq::CDKGDebugManager& dkg_debugman,
                                 llmq::CQuorumBlockProcessor& qblockman, llmq::CQuorumManager& qman, llmq::CQuorumSnapshotManager& qsnapman,
                                 const CSporkManager& sporkman, const util::DbWrapperParams& db_params, bool quorums_recovery) :
    qdkgsman{std::make_unique<llmq::CDKGSessionManager>(chainstate, dmnman, qsnapman, sporkman,
                                                        [&](llmq::CDKGSessionManager::SessionHandlerMap& map,
                                                            const Consensus::LLMQParams& llmq_params, int quorum_idx) -> void {
                                                            map.emplace(llmq::CDKGSessionManager::SessionHandlerKey{llmq_params.type, quorum_idx},
                                                                std::make_unique<llmq::CDKGSessionHandler>(
                                                                    bls_worker, chainstate, dmnman, dkg_debugman, mn_metaman, qblockman, qsnapman,
                                                                    /*mn_activeman=*/nullptr, sporkman, qdkgsman, llmq_params, /*quorums_watch=*/true,
                                                                    quorum_idx));
                                                        }, db_params, /*quorums_watch=*/true)},
    qman_handler{std::make_unique<llmq::QuorumObserver>(dmnman, qman, qsnapman, mn_sync, sporkman, quorums_recovery)},
    m_qman{qman}
{
    m_qman.ConnectManagers(qman_handler.get(), qdkgsman.get());
}

ObserverContext::~ObserverContext()
{
    m_qman.DisconnectManagers();
}
} // namespace llmq

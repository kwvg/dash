// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_PARTICIPANT_H
#define BITCOIN_LLMQ_PARTICIPANT_H

#include <bls/bls.h>
#include <llmq/quorums.h>
#include <llmq/types.h>
#include <msg_result.h>

#include <consensus/params.h>
#include <saltedhasher.h>
#include <span.h>
#include <sync.h>
#include <threadsafety.h>
#include <uint256.h>

#include <map>

class CActiveMasternodeManager;
class CBlockIndex;
class CBLSWorker;
class CConnman;
class CDeterministicMNManager;
class CDKGSessionManager;
class CNode;
class CSporkManager;

namespace llmq {
class CQuorum;
class CQuorumDataRequest;
class CQuorumManager;
class CQuorumSnapshotManager;
enum class QvvecSyncMode : int8_t;

class QuorumParticipant
{
private:
    CBLSWorker& m_bls_worker;
    CDeterministicMNManager& m_dmnman;
    CDKGSessionManager& m_dkgsman;
    CQuorumManager& m_qman;
    CQuorumSnapshotManager& m_qsnapman;
    const CActiveMasternodeManager* const m_mn_activeman;
    const CMasternodeSync& m_mn_sync;
    const CSporkManager& m_sporkman;

    const bool m_quorums_recovery{false};
    const bool m_quorums_watch{false};

    mutable Mutex cs_cleanup;
    mutable std::map<Consensus::LLMQType, Uint256LruHashMap<uint256>> cleanupQuorumsCache GUARDED_BY(cs_cleanup);

public:
    QuorumParticipant() = delete;
    QuorumParticipant(const QuorumParticipant&) = delete;
    QuorumParticipant& operator=(const QuorumParticipant&) = delete;
    explicit QuorumParticipant(CBLSWorker& bls_worker, CDeterministicMNManager& dmnman, CDKGSessionManager& dkgsman, CQuorumManager& qman,
                               CQuorumSnapshotManager& qsnapman, const CActiveMasternodeManager* const mn_activeman,
                               const CMasternodeSync& mn_sync, const CSporkManager& sporkman, bool quorums_recovery, bool quorums_watch);
    ~QuorumParticipant();

    void UpdatedBlockTip(const CBlockIndex* pindexNew, CConnman& connman, bool fInitialDownload) const;

    [[nodiscard]] MessageProcessingResult ProcessEncryptedContribs(CNode& pfrom, CConnman& connman, bool request_limit_exceeded,
                                                                   CDataStream& vStream, CQuorum& quorum, CQuorumDataRequest& request,
                                                                   const CBlockIndex* const block_index, std::string_view msg_type);

    bool SetQuorumSecretKeyShare(CQuorum& quorum, Span<CBLSSecretKey> skContributions) const;

private:
    void TriggerQuorumDataRecoveryThreads(CConnman& connman, gsl::not_null<const CBlockIndex*> pIndex) const;

    void CheckQuorumConnections(CConnman& connman, const Consensus::LLMQParams& llmqParams, gsl::not_null<const CBlockIndex*> pindexNew) const;
    void CheckQuorumConnectionsMn(CConnman& connman, const Consensus::LLMQParams& llmqParams, gsl::not_null<const CBlockIndex*> pindexNew) const;
    void CheckQuorumConnectionsWatchOnly(CConnman& connman, const Consensus::LLMQParams& llmqParams, gsl::not_null<const CBlockIndex*> pindexNew) const;

    /// Returns the start offset for the masternode with the given proTxHash. This offset is applied when picking data recovery members of a quorum's
    /// memberlist and is calculated based on a list of all member of all active quorums for the given llmqType in a way that each member
    /// should receive the same number of request if all active llmqType members requests data from one llmqType quorum.
    size_t GetQuorumRecoveryStartOffset(const CQuorum& quorum, gsl::not_null<const CBlockIndex*> pIndex) const;

    Uint256HashSet GetQuorumsToDelete(CConnman& connman, const Consensus::LLMQParams& llmqParams, gsl::not_null<const CBlockIndex*> pindexNew) const;

    void StartDataRecoveryThread(CConnman& connman, gsl::not_null<const CBlockIndex*> pIndex, CQuorumCPtr pQuorum, uint16_t nDataMask) const;
    void TriggerDataRecoveryThreads(CConnman& connman, gsl::not_null<const CBlockIndex*> block_index) const;

    void StartVvecSyncThread(CConnman& connman, gsl::not_null<const CBlockIndex*> block_index, CQuorumCPtr pQuorum) const;
    void TriggerVvecSyncThreads(CConnman& connman, gsl::not_null<const CBlockIndex*> block_index) const;
    void TryStartVvecSyncThread(CConnman& connman, gsl::not_null<const CBlockIndex*> block_index, CQuorumCPtr pQuorum,
                                const std::map<Consensus::LLMQType, QvvecSyncMode>& mapQuorumVvecSync, bool fWeAreQuorumTypeMember) const;

    void StartCleanupOldQuorumDataThread(gsl::not_null<const CBlockIndex*> pIndex) const;

    void DataRecoveryThread(CConnman& connman, gsl::not_null<const CBlockIndex*> block_index, CQuorumCPtr quorum, uint16_t data_mask, const uint256& protx_hash, size_t start_offset) const;
};
} // namespace llmq

#endif // BITCOIN_LLMQ_PARTICIPANT_H

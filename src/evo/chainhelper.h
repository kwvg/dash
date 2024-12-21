// Copyright (c) 2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_CHAINHELPER_H
#define BITCOIN_EVO_CHAINHELPER_H

#include <uint256.h>

#include <memory>
#include <optional>

class CCreditPoolManager;
class CDeterministicMNManager;
class ChainstateManager;
class CMNHFManager;
class CMNPaymentsProcessor;
class CMasternodeSync;
class CGovernanceManager;
class CSpecialTxProcessor;
class CSporkManager;
class CTransaction;

namespace Consensus { struct Params; }
namespace llmq {
class CChainLocksHandler;
class CInstantSendManager;
class CQuorumBlockProcessor;
class CQuorumManager;
}

class CChainstateHelper
{
private:
    llmq::CInstantSendManager& isman;
    const llmq::CChainLocksHandler& clhandler;

public:
    explicit CChainstateHelper(CCreditPoolManager& cpoolman, CDeterministicMNManager& dmnman, CMNHFManager& mnhfman, CGovernanceManager& govman,
                               llmq::CInstantSendManager& isman, llmq::CQuorumBlockProcessor& qblockman, const ChainstateManager& chainman,
                               const Consensus::Params& consensus_params, const CMasternodeSync& mn_sync, const CSporkManager& sporkman,
                               const llmq::CChainLocksHandler& clhandler, const llmq::CQuorumManager& qman);
    ~CChainstateHelper();

    CChainstateHelper() = delete;
    CChainstateHelper(const CChainstateHelper&) = delete;

    /** Passthrough functions to CChainLocksHandler */
    bool HasConflictingChainLock(int nHeight, const uint256& blockHash) const;
    bool HasChainLock(int nHeight, const uint256& blockHash) const;
    int32_t GetBestChainLockHeight() const;

    /** Passthrough functions to CInstantSendManager */
    std::optional<
        std::pair</*islock_hash=*/uint256, /*txid=*/uint256>
    > HasConflictingISLock(const CTransaction& tx) const;
    bool IsInstantSendWaitingForTx(const uint256& hash) const;
    bool RemoveConflictingISLockByTx(const CTransaction& tx);
    bool ShouldInstantSendRejectConflicts() const;

public:
    const std::unique_ptr<CMNPaymentsProcessor> mn_payments;
    const std::unique_ptr<CSpecialTxProcessor> special_tx;
};

#endif // BITCOIN_EVO_CHAINHELPER_H

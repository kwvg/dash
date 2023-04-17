// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validationinterface.h>

#include <chain.h>
#include <consensus/validation.h>
#include <governance/object.h>
#include <evo/deterministicmns.h>
#include <llmq/clsig.h>
#include <llmq/instantsend.h>
#include <llmq/signing.h>
#include <logging.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <scheduler.h>
#include <util/validation.h>

#include <future>
#include <unordered_map>
#include <utility>

#include <boost/signals2/signal.hpp>

struct ValidationInterfaceConnections {
    boost::signals2::scoped_connection UpdatedBlockTip;
    boost::signals2::scoped_connection SynchronousUpdatedBlockTip;
    boost::signals2::scoped_connection TransactionAddedToMempool;
    boost::signals2::scoped_connection BlockConnected;
    boost::signals2::scoped_connection BlockDisconnected;
    boost::signals2::scoped_connection TransactionRemovedFromMempool;
    boost::signals2::scoped_connection ChainStateFlushed;
    boost::signals2::scoped_connection BlockChecked;
    boost::signals2::scoped_connection NewPoWValidBlock;
    boost::signals2::scoped_connection AcceptedBlockHeader;
    boost::signals2::scoped_connection NotifyHeaderTip;
    boost::signals2::scoped_connection NotifyTransactionLock;
    boost::signals2::scoped_connection NotifyChainLock;
    boost::signals2::scoped_connection NotifyGovernanceVote;
    boost::signals2::scoped_connection NotifyGovernanceObject;
    boost::signals2::scoped_connection NotifyInstantSendDoubleSpendAttempt;
    boost::signals2::scoped_connection NotifyMasternodeListChanged;
    boost::signals2::scoped_connection NotifyRecoveredSig;

};

struct MainSignalsInstance {
    boost::signals2::signal<void (const CBlockIndex *, const CBlockIndex *, bool fInitialDownload)> UpdatedBlockTip;
    boost::signals2::signal<void (const CBlockIndex *, const CBlockIndex *, bool fInitialDownload)> SynchronousUpdatedBlockTip;
    boost::signals2::signal<void (const CTransactionRef &, int64_t)> TransactionAddedToMempool;
    boost::signals2::signal<void (const std::shared_ptr<const CBlock> &, const CBlockIndex *pindex)> BlockConnected;
    boost::signals2::signal<void (const std::shared_ptr<const CBlock>&, const CBlockIndex* pindex)> BlockDisconnected;
    boost::signals2::signal<void (const CTransactionRef &, MemPoolRemovalReason)> TransactionRemovedFromMempool;
    boost::signals2::signal<void (const CBlockLocator &)> ChainStateFlushed;
    boost::signals2::signal<void (const CBlock&, const BlockValidationState&)> BlockChecked;
    boost::signals2::signal<void (const CBlockIndex *, const std::shared_ptr<const CBlock>&)> NewPoWValidBlock;
    boost::signals2::signal<void (const CBlockIndex *)>AcceptedBlockHeader;
    boost::signals2::signal<void (const CBlockIndex *, bool)>NotifyHeaderTip;
    boost::signals2::signal<void (const CTransactionRef& tx, const std::shared_ptr<const llmq::CInstantSendLock>& islock)>NotifyTransactionLock;
    boost::signals2::signal<void (const CBlockIndex* pindex, const std::shared_ptr<const llmq::CChainLockSig>& clsig)>NotifyChainLock;
    boost::signals2::signal<void (const std::shared_ptr<const CGovernanceVote>& vote)>NotifyGovernanceVote;
    boost::signals2::signal<void (const std::shared_ptr<const CGovernanceObject>& object)>NotifyGovernanceObject;
    boost::signals2::signal<void (const CTransactionRef& currentTx, const CTransactionRef& previousTx)>NotifyInstantSendDoubleSpendAttempt;
    boost::signals2::signal<void (bool undo, const CDeterministicMNList& oldMNList, const CDeterministicMNListDiff& diff, CConnman& connman)>NotifyMasternodeListChanged;
    boost::signals2::signal<void (const std::shared_ptr<const llmq::CRecoveredSig>& sig)>NotifyRecoveredSig;
    // We are not allowed to assume the scheduler only runs in one thread,
    // but must ensure all callbacks happen in-order, so we end up creating
    // our own queue here :(
    SingleThreadedSchedulerClient m_schedulerClient;
    std::unordered_map<CValidationInterface*, ValidationInterfaceConnections> m_connMainSignals;

    explicit MainSignalsInstance(CScheduler *pscheduler) : m_schedulerClient(pscheduler) {}
};

static CMainSignals g_signals;

void CMainSignals::RegisterBackgroundSignalScheduler(CScheduler& scheduler) {
    assert(!m_internals);
    m_internals.reset(new MainSignalsInstance(&scheduler));
}

void CMainSignals::UnregisterBackgroundSignalScheduler() {
    m_internals.reset(nullptr);
}

void CMainSignals::FlushBackgroundCallbacks() {
    if (m_internals) {
        m_internals->m_schedulerClient.EmptyQueue();
    }
}

size_t CMainSignals::CallbacksPending() {
    if (!m_internals) return 0;
    return m_internals->m_schedulerClient.CallbacksPending();
}

CMainSignals& GetMainSignals()
{
    return g_signals;
}

void RegisterSharedValidationInterface(std::shared_ptr<CValidationInterface> pwalletIn) {
    // Each connection captures pwalletIn to ensure that each callback is
    // executed before pwalletIn is destroyed. For more details see #18338.
    ValidationInterfaceConnections& conns = g_signals.m_internals->m_connMainSignals[pwalletIn.get()];
    conns.AcceptedBlockHeader = g_signals.m_internals->AcceptedBlockHeader.connect(std::bind(&CValidationInterface::AcceptedBlockHeader, pwalletIn, std::placeholders::_1));
    conns.NotifyHeaderTip = g_signals.m_internals->NotifyHeaderTip.connect(std::bind(&CValidationInterface::NotifyHeaderTip, pwalletIn, std::placeholders::_1, std::placeholders::_2));
    conns.UpdatedBlockTip = g_signals.m_internals->UpdatedBlockTip.connect(std::bind(&CValidationInterface::UpdatedBlockTip, pwalletIn, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    conns.SynchronousUpdatedBlockTip = g_signals.m_internals->SynchronousUpdatedBlockTip.connect(std::bind(&CValidationInterface::SynchronousUpdatedBlockTip, pwalletIn, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    conns.TransactionAddedToMempool = g_signals.m_internals->TransactionAddedToMempool.connect(std::bind(&CValidationInterface::TransactionAddedToMempool, pwalletIn, std::placeholders::_1, std::placeholders::_2));
    conns.BlockConnected = g_signals.m_internals->BlockConnected.connect(std::bind(&CValidationInterface::BlockConnected, pwalletIn, std::placeholders::_1, std::placeholders::_2));
    conns.BlockDisconnected = g_signals.m_internals->BlockDisconnected.connect(std::bind(&CValidationInterface::BlockDisconnected, pwalletIn, std::placeholders::_1, std::placeholders::_2));
    conns.NotifyTransactionLock = g_signals.m_internals->NotifyTransactionLock.connect(std::bind(&CValidationInterface::NotifyTransactionLock, pwalletIn, std::placeholders::_1, std::placeholders::_2));
    conns.NotifyChainLock = g_signals.m_internals->NotifyChainLock.connect(std::bind(&CValidationInterface::NotifyChainLock, pwalletIn, std::placeholders::_1, std::placeholders::_2));
    conns.TransactionRemovedFromMempool = g_signals.m_internals->TransactionRemovedFromMempool.connect(std::bind(&CValidationInterface::TransactionRemovedFromMempool, pwalletIn, std::placeholders::_1, std::placeholders::_2));
    conns.ChainStateFlushed = g_signals.m_internals->ChainStateFlushed.connect(std::bind(&CValidationInterface::ChainStateFlushed, pwalletIn, std::placeholders::_1));
    conns.BlockChecked = g_signals.m_internals->BlockChecked.connect(std::bind(&CValidationInterface::BlockChecked, pwalletIn, std::placeholders::_1, std::placeholders::_2));
    conns.NewPoWValidBlock = g_signals.m_internals->NewPoWValidBlock.connect(std::bind(&CValidationInterface::NewPoWValidBlock, pwalletIn, std::placeholders::_1, std::placeholders::_2));
    conns.NotifyGovernanceObject = g_signals.m_internals->NotifyGovernanceObject.connect(std::bind(&CValidationInterface::NotifyGovernanceObject, pwalletIn, std::placeholders::_1));
    conns.NotifyGovernanceVote = g_signals.m_internals->NotifyGovernanceVote.connect(std::bind(&CValidationInterface::NotifyGovernanceVote, pwalletIn, std::placeholders::_1));
    conns.NotifyInstantSendDoubleSpendAttempt = g_signals.m_internals->NotifyInstantSendDoubleSpendAttempt.connect(std::bind(&CValidationInterface::NotifyInstantSendDoubleSpendAttempt, pwalletIn, std::placeholders::_1, std::placeholders::_2));
    conns.NotifyRecoveredSig = g_signals.m_internals->NotifyRecoveredSig.connect(std::bind(&CValidationInterface::NotifyRecoveredSig, pwalletIn, std::placeholders::_1));
    conns.NotifyMasternodeListChanged = g_signals.m_internals->NotifyMasternodeListChanged.connect(std::bind(&CValidationInterface::NotifyMasternodeListChanged, pwalletIn, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}

void RegisterValidationInterface(CValidationInterface* callbacks)
{
    // Create a shared_ptr with a no-op deleter - CValidationInterface lifecycle
    // is managed by the caller.
    RegisterSharedValidationInterface({callbacks, [](CValidationInterface*){}});
}

void UnregisterSharedValidationInterface(std::shared_ptr<CValidationInterface> callbacks)
{
    UnregisterValidationInterface(callbacks.get());
}

void UnregisterValidationInterface(CValidationInterface* pwalletIn) {
    if (g_signals.m_internals) {
        g_signals.m_internals->m_connMainSignals.erase(pwalletIn);
    }
}

void UnregisterAllValidationInterfaces() {
    if (!g_signals.m_internals) {
        return;
    }
    g_signals.m_internals->m_connMainSignals.clear();
}

void CallFunctionInValidationInterfaceQueue(std::function<void ()> func) {
    g_signals.m_internals->m_schedulerClient.AddToProcessQueue(std::move(func));
}

void SyncWithValidationInterfaceQueue() {
    AssertLockNotHeld(cs_main);
    // Block until the validation queue drains
    std::promise<void> promise;
    CallFunctionInValidationInterfaceQueue([&promise] {
        promise.set_value();
    });
    promise.get_future().wait();
}

// Use a macro instead of a function for conditional logging to prevent
// evaluating arguments when logging is not enabled.
//
// NOTE: The lambda captures all local variables by value.
#define ENQUEUE_AND_LOG_EVENT(event, fmt, name, ...)           \
    do {                                                       \
        auto local_name = (name);                              \
        LOG_EVENT("Enqueuing " fmt, local_name, __VA_ARGS__);  \
        m_internals->m_schedulerClient.AddToProcessQueue([=] { \
            LOG_EVENT(fmt, local_name, __VA_ARGS__);           \
            event();                                           \
        });                                                    \
    } while (0)

#define LOG_EVENT(fmt, ...) \
    LogPrint(BCLog::VALIDATION, fmt "\n", __VA_ARGS__)

void CMainSignals::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) {
    // Dependencies exist that require UpdatedBlockTip events to be delivered in the order in which
    // the chain actually updates. One way to ensure this is for the caller to invoke this signal
    // in the same critical section where the chain is updated

    auto event = [pindexNew, pindexFork, fInitialDownload, this] {
        m_internals->UpdatedBlockTip(pindexNew, pindexFork, fInitialDownload);
    };
    ENQUEUE_AND_LOG_EVENT(event, "%s: new block hash=%s fork block hash=%s (in IBD=%s)", __func__,
                          pindexNew->GetBlockHash().ToString(),
                          pindexFork ? pindexFork->GetBlockHash().ToString() : "null",
                          fInitialDownload);
}

void CMainSignals::SynchronousUpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) {
    auto event = [pindexNew, pindexFork, fInitialDownload, this] {
        m_internals->SynchronousUpdatedBlockTip(pindexNew, pindexFork, fInitialDownload);
    };
    ENQUEUE_AND_LOG_EVENT(event, "%s: block hash=%s fork block hash=%s (in IBD=%s)", __func__,
                          pindexNew->GetBlockHash().ToString(),
                          pindexFork ? pindexFork->GetBlockHash().ToString() : "null",
                          fInitialDownload);
}

void CMainSignals::TransactionAddedToMempool(const CTransactionRef &ptx, int64_t nAcceptTime) {
    auto event = [ptx, nAcceptTime, this] {
        m_internals->TransactionAddedToMempool(ptx, nAcceptTime);
    };
    ENQUEUE_AND_LOG_EVENT(event, "%s: txid=%s", __func__,
                          ptx->GetHash().ToString());
}

void CMainSignals::TransactionRemovedFromMempool(const CTransactionRef &ptx, MemPoolRemovalReason reason) {
    auto event = [ptx, reason, this] {
        m_internals->TransactionRemovedFromMempool(ptx, reason);
    };
    ENQUEUE_AND_LOG_EVENT(event, "%s: txid=%s", __func__,
                          ptx->GetHash().ToString());
}

void CMainSignals::BlockConnected(const std::shared_ptr<const CBlock> &pblock, const CBlockIndex *pindex) {
    auto event = [pblock, pindex, this] {
        m_internals->BlockConnected(pblock, pindex);
    };
    ENQUEUE_AND_LOG_EVENT(event, "%s: block hash=%s block height=%d", __func__,
                          pblock->GetHash().ToString(),
                          pindex->nHeight);
}

void CMainSignals::BlockDisconnected(const std::shared_ptr<const CBlock> &pblock, const CBlockIndex* pindex) {
    auto event = [pblock, pindex, this] {
        m_internals->BlockDisconnected(pblock, pindex);
    };
    ENQUEUE_AND_LOG_EVENT(event, "%s: block hash=%s block height=%d", __func__,
                          pblock->GetHash().ToString(),
                          pindex->nHeight);
}

void CMainSignals::ChainStateFlushed(const CBlockLocator &locator) {
    auto event = [locator, this] {
        m_internals->ChainStateFlushed(locator);
    };
    ENQUEUE_AND_LOG_EVENT(event, "%s: block hash=%s", __func__,
                          locator.IsNull() ? "null" : locator.vHave.front().ToString());
}

void CMainSignals::BlockChecked(const CBlock& block, const BlockValidationState& state) {
    LOG_EVENT("%s: block hash=%s state=%s", __func__,
              block.GetHash().ToString(), FormatStateMessage(state));
    m_internals->BlockChecked(block, state);
}

void CMainSignals::NewPoWValidBlock(const CBlockIndex *pindex, const std::shared_ptr<const CBlock> &block) {
    LOG_EVENT("%s: block hash=%s", __func__, block->GetHash().ToString());
    m_internals->NewPoWValidBlock(pindex, block);
}

void CMainSignals::AcceptedBlockHeader(const CBlockIndex *pindexNew) {
    LOG_EVENT("%s: block hash=%s", __func__, pindexNew->GetBlockHash().ToString());
    m_internals->AcceptedBlockHeader(pindexNew);
}

void CMainSignals::NotifyHeaderTip(const CBlockIndex *pindexNew, bool fInitialDownload) {
    LOG_EVENT("%s: block hash=%s", __func__, pindexNew->GetBlockHash().ToString());
    m_internals->NotifyHeaderTip(pindexNew, fInitialDownload);
}

void CMainSignals::NotifyTransactionLock(const CTransactionRef &tx, const std::shared_ptr<const llmq::CInstantSendLock>& islock) {
    auto event = [tx, islock, this] {
        m_internals->NotifyTransactionLock(tx, islock);
    };
    ENQUEUE_AND_LOG_EVENT(event, "%s: islock txid=%s islock cycle=%s islock sig=%s", __func__,
                          islock->txid.ToString(),
                          islock->cycleHash.ToString(),
                          islock->sig.GetHash().ToString());
}

void CMainSignals::NotifyChainLock(const CBlockIndex* pindex, const std::shared_ptr<const llmq::CChainLockSig>& clsig) {
    auto event = [pindex, clsig, this] {
        m_internals->NotifyChainLock(pindex, clsig);
    };
    ENQUEUE_AND_LOG_EVENT(event, "%s: cl height=%d cl hash=%s cl sig=%s", __func__,
                          clsig->getHeight(),
                          clsig->getBlockHash().ToString(),
                          clsig->getSig().GetHash().ToString());
}

void CMainSignals::NotifyGovernanceVote(const std::shared_ptr<const CGovernanceVote>& vote) {
    auto event = [vote, this] {
        m_internals->NotifyGovernanceVote(vote);
    };
    ENQUEUE_AND_LOG_EVENT(event, "%s: vote time=%d parent hash=%s vote hash=%s", __func__,
                          vote->GetTimestamp(),
                          vote->GetParentHash().ToString(),
                          vote->GetHash().ToString());
}

void CMainSignals::NotifyGovernanceObject(const std::shared_ptr<const CGovernanceObject>& object) {
    auto event = [object, this] {
        m_internals->NotifyGovernanceObject(object);
    };
    ENQUEUE_AND_LOG_EVENT(event, "%s: creation time=%d deletion time=%d collateral hash=%s", __func__,
                          object->GetCreationTime(),
                          object->GetDeletionTime(),
                          object->GetCollateralHash().ToString());
}

void CMainSignals::NotifyInstantSendDoubleSpendAttempt(const CTransactionRef& currentTx, const CTransactionRef& previousTx) {
    auto event = [currentTx, previousTx, this] {
        m_internals->NotifyInstantSendDoubleSpendAttempt(currentTx, previousTx);
    };
    ENQUEUE_AND_LOG_EVENT(event, "%s: current tx=%s prev tx=%d", __func__,
                          currentTx->GetHash().ToString(),
                          previousTx->GetHash().ToString());
}

void CMainSignals::NotifyRecoveredSig(const std::shared_ptr<const llmq::CRecoveredSig>& sig) {
    auto event = [sig, this] {
        m_internals->NotifyRecoveredSig(sig);
    };
    ENQUEUE_AND_LOG_EVENT(event, "%s: quorum hash=%s id hash=%s msg hash=%s sig hash=%s", __func__,
                          sig->getQuorumHash().ToString(),
                          sig->getId().ToString(),
                          sig->getMsgHash().ToString(),
                          sig->sig.GetHash().ToString());
}

void CMainSignals::NotifyMasternodeListChanged(bool undo, const CDeterministicMNList& oldMNList, const CDeterministicMNListDiff& diff, CConnman& connman) {
    LOG_EVENT("%s: old block hash=%s old block height=%d old registered count=%d "
              "diff added=%d diff updated=%s diff removed=%d", __func__, 
              oldMNList.GetBlockHash().ToString(),
              oldMNList.GetHeight(),
              oldMNList.GetTotalRegisteredCount(),
              diff.addedMNs.size(),
              diff.updatedMNs.size(),
              diff.removedMns.size());
    m_internals->NotifyMasternodeListChanged(undo, oldMNList, diff, connman);    
}

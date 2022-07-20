// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_CONTEXT_H
#define BITCOIN_NODE_CONTEXT_H

#include <cassert>
#include <functional>
#include <memory>
#include <vector>

class ArgsManager;
class BanMan;
class CBLSWorker;
class CEvoDB;
class CConnman;
class CScheduler;
class CTxMemPool;
class ChainstateManager;
class PeerLogicValidation;
struct NodeContext;
namespace interfaces {
class Chain;
class ChainClient;
class WalletClient;
} // namespace interfaces

namespace llmq {
class CDKGDebugManager;
class CQuorumBlockProcessor;
class CDKGSessionManager;
class CQuorumManager;
class CSigSharesManager;
class CSigningManager;
class CChainLocksHandler;
class CInstantSendManager;

struct Context {
    std::shared_ptr<CBLSWorker> blsWorker;                          /* shared between quorumDKGSessionManager and quorumManager */
    std::unique_ptr<CDKGDebugManager> quorumDKGDebugManager;
    std::unique_ptr<CQuorumBlockProcessor> quorumBlockProcessor;
    std::unique_ptr<CDKGSessionManager> quorumDKGSessionManager;
    std::unique_ptr<CQuorumManager> quorumManager;
    std::unique_ptr<CSigSharesManager> quorumSigSharesManager;
    std::unique_ptr<CSigningManager> quorumSigningManager;
    std::unique_ptr<CChainLocksHandler> chainLocksHandler;
    std::unique_ptr<CInstantSendManager> quorumInstantSendManager;

    //! Declare default constructor and destructor that are not inline, so code
    //! instantiating the llmq::Context struct doesn't need to #include class
    //! definitions for all the unique_ptr members.
    Context(NodeContext& node, CEvoDB& evoDb, CTxMemPool& mempool, CConnman& connman, bool unitTests, bool fWipe);
    ~Context();
};
}

//! NodeContext struct containing references to chain state and connection
//! state.
//!
//! This is used by init, rpc, and test code to pass object references around
//! without needing to declare the same variables and parameters repeatedly, or
//! to use globals. More variables could be added to this struct (particularly
//! references to validation objects) to eliminate use of globals
//! and make code more modular and testable. The struct isn't intended to have
//! any member functions. It should just be a collection of references that can
//! be used without pulling in unwanted dependencies or functionality.
struct NodeContext {
    std::unique_ptr<CConnman> connman;
    CTxMemPool* mempool{nullptr}; // Currently a raw pointer because the memory is not managed by this struct
    std::unique_ptr<PeerLogicValidation> peer_logic;
    ChainstateManager* chainman{nullptr}; // Currently a raw pointer because the memory is not managed by this struct
    std::unique_ptr<BanMan> banman;
    ArgsManager* args{nullptr}; // Currently a raw pointer because the memory is not managed by this struct
    std::unique_ptr<interfaces::Chain> chain;
    //! List of all chain clients (wallet processes or other client) connected to node.
    std::vector<std::unique_ptr<interfaces::ChainClient>> chain_clients;
    //! Reference to chain client that should used to load or create wallets
    //! opened by the gui.
    interfaces::WalletClient* wallet_client{nullptr};
    std::unique_ptr<CScheduler> scheduler;
    std::function<void()> rpc_interruption_point = [] {};
    std::unique_ptr<llmq::Context> llmq_ctx;

    //! Declare default constructor and destructor that are not inline, so code
    //! instantiating the NodeContext struct doesn't need to #include class
    //! definitions for all the unique_ptr members.
    NodeContext();
    ~NodeContext();
};

#endif // BITCOIN_NODE_CONTEXT_H

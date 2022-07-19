// Copyright (c) 2018-2022 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_INIT_H
#define BITCOIN_LLMQ_INIT_H

class CConnman;
class CDBWrapper;
class CEvoDB;
class CTxMemPool;
class NodeContext;

namespace llmq
{

// Init/destroy LLMQ globals
void InitLLMQSystem(NodeContext& node, CEvoDB& evoDb, CTxMemPool& mempool, CConnman& connman, bool unitTests, bool fWipe = false);
void DestroyLLMQSystem();

// Manage scheduled tasks, threads, listeners etc.
void StartLLMQSystem(NodeContext& node);
void StopLLMQSystem(NodeContext& node);
void InterruptLLMQSystem(NodeContext& node);
} // namespace llmq

#endif // BITCOIN_LLMQ_INIT_H

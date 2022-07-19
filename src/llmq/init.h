// Copyright (c) 2018-2022 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_INIT_H
#define BITCOIN_LLMQ_INIT_H

class CConnman;
class CDBWrapper;
class CEvoDB;
class CTxMemPool;

namespace dash { struct Context; }

namespace llmq
{

// Init/destroy LLMQ globals
void InitLLMQSystem(CEvoDB& evoDb, CTxMemPool& mempool, CConnman& connman, dash::Context& ctx, bool unitTests, bool fWipe = false);
void DestroyLLMQSystem();

// Manage scheduled tasks, threads, listeners etc.
void StartLLMQSystem();
void StopLLMQSystem();
void InterruptLLMQSystem();
} // namespace llmq

#endif // BITCOIN_LLMQ_INIT_H

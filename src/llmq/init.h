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
void DestroyLLMQSystem();

// Manage scheduled tasks, threads, listeners etc.
void StartLLMQSystem(Context &ctx);
void StopLLMQSystem(Context &ctx);
void InterruptLLMQSystem(Context &ctx);
} // namespace llmq

#endif // BITCOIN_LLMQ_INIT_H

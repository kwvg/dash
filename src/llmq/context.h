// Copyright (c) 2018-2022 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_CONTEXT_H
#define BITCOIN_LLMQ_CONTEXT_H

class CConnman;
class CDBWrapper;
class CEvoDB;
class CTxMemPool;
class CSporkManager;

struct LLMQContext {
    LLMQContext(CEvoDB& evoDb, CTxMemPool& mempool, CConnman& connman, CSporkManager& sporkManager, bool unitTests, bool fWipe);
    ~LLMQContext();

    void Create(CEvoDB& evoDb, CTxMemPool& mempool, CConnman& connman, CSporkManager& sporkManager, bool unitTests, bool fWipe);
    void Destroy();
    void Interrupt();
    void Start();
    void Stop();
};

#endif // BITCOIN_LLMQ_CONTEXT_H

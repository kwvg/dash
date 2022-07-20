// Copyright (c) 2018-2022 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_SPECIALTXMAN_H
#define BITCOIN_EVO_SPECIALTXMAN_H

#include <primitives/transaction.h>
#include <sync.h>
#include <threadsafety.h>

class CBlock;
class CBlockIndex;
class CCoinsViewCache;
class CValidationState;

extern CCriticalSection cs_main;

namespace llmq {
    struct Context;
    class CSigningManager;
    class CQuorumBlockProcessor;
}

bool CheckSpecialTx(const llmq::Context& ctx, const CTransaction& tx, const CBlockIndex* pindexPrev, CValidationState& state, const CCoinsViewCache& view, bool check_sigs) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
bool ProcessSpecialTxsInBlock(const llmq::Context& ctx, const CBlock& block, const CBlockIndex* pindex, CValidationState& state, const CCoinsViewCache& view, bool fJustCheck, bool fCheckCbTxMerleRoots) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
bool UndoSpecialTxsInBlock(const llmq::Context& ctx, const CBlock& block, const CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

#endif // BITCOIN_EVO_SPECIALTXMAN_H

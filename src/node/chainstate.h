// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_CHAINSTATE_H
#define BITCOIN_NODE_CHAINSTATE_H

#include <cstdint> // for int64_t
#include <optional> // for std::optional

class ArgsManager;
class CChainParams;
class ChainstateManager;
struct NodeContext;

enum class ChainstateLoadingError {
    ERROR_LOADING_BLOCK_DB,
    ERROR_BAD_GENESIS_BLOCK,
    ERROR_BAD_DEVNET_GENESIS_BLOCK,
    ERROR_TXINDEX_DISABLED_WHEN_GOV_ENABLED,
    ERROR_ADDRIDX_NEEDS_REINDEX,
    ERROR_SPENTIDX_NEEDS_REINDEX,
    ERROR_TIMEIDX_NEEDS_REINDEX,
    ERROR_PRUNED_NEEDS_REINDEX,
    ERROR_LOAD_GENESIS_BLOCK_FAILED,
    ERROR_CHAINSTATE_UPGRADE_FAILED,
    ERROR_REPLAYBLOCKS_FAILED,
    ERROR_LOADCHAINTIP_FAILED,
    ERROR_EVO_DB_SANITY_FAILED,
    ERROR_GENERIC_BLOCKDB_OPEN_FAILED,
    ERROR_BLOCK_FROM_FUTURE,
    ERROR_CORRUPTED_BLOCK_DB,
    ERROR_COMMITING_EVO_DB,
    ERROR_UPGRADING_EVO_DB,
    SHUTDOWN_PROBED,
};

/** This sequence can have 4 types of outcomes:
 *
 *  1. Success
 *  2. Shutdown requested
 *    - nothing failed but a shutdown was triggered in the middle of the
 *      sequence
 *  3. Soft failure
 *    - a failure that might be recovered from with a reindex
 *  4. Hard failure
 *    - a failure that definitively cannot be recovered from with a reindex
 *
 *  Currently, LoadChainstate returns a std::optional<ChainstateLoadingError>
 *  which:
 *
 *  - if has_value()
 *      - Either "Soft failure", "Hard failure", or "Shutdown requested",
 *        differentiable by the specific enumerator.
 *
 *        Note that a return value of SHUTDOWN_PROBED means ONLY that "during
 *        this sequence, when we explicitly checked ShutdownRequested() at
 *        arbitrary points, one of those calls returned true". Therefore, a
 *        return value other than SHUTDOWN_PROBED does not guarantee that
 *        ShutdownRequested() hasn't been called indirectly.
 *  - else
 *      - Success!
 */
std::optional<ChainstateLoadingError> LoadChainstate(bool fReset,
                                                     ChainstateManager& chainman,
                                                     NodeContext& node,
                                                     bool fPruneMode,
                                                     bool is_governance_enabled,
                                                     const CChainParams& chainparams,
                                                     const ArgsManager& args,
                                                     bool fReindexChainState,
                                                     int64_t nBlockTreeDBCache,
                                                     int64_t nCoinDBCache,
                                                     int64_t nCoinCacheUsage);

#endif // BITCOIN_NODE_CHAINSTATE_H

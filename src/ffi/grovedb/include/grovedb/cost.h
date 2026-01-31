// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GROVEDB_COST_H
#define GROVEDB_COST_H

#include <cstdint>

namespace grovedb {

/**
 * Resource consumption counters returned alongside cost-tracked operations.
 */
struct OperationCost {
    uint32_t m_seek_count{0};
    uint32_t m_storage_added_bytes{0};
    uint32_t m_storage_replaced_bytes{0};
    uint32_t m_storage_removed_bytes{0};
    uint64_t m_storage_loaded_bytes{0};
    uint32_t m_hash_node_calls{0};
};

} // namespace grovedb

#endif // GROVEDB_COST_H

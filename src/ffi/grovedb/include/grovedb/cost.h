// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#ifndef LIBGROVEDB_COST_H
#define LIBGROVEDB_COST_H

#include <compare>
#include <cstdint>
#include <tuple>

namespace grovedb {
/** @addtogroup cost
 *  @{ */
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

  /** Accumulate another cost into this one. */
  constexpr OperationCost& operator+=(const OperationCost& rhs)
  {
    m_seek_count += rhs.m_seek_count;
    m_storage_added_bytes += rhs.m_storage_added_bytes;
    m_storage_replaced_bytes += rhs.m_storage_replaced_bytes;
    m_storage_removed_bytes += rhs.m_storage_removed_bytes;
    m_storage_loaded_bytes += rhs.m_storage_loaded_bytes;
    m_hash_node_calls += rhs.m_hash_node_calls;
    return *this;
  }

  [[nodiscard]] constexpr bool operator==(const OperationCost& rhs) const = default;
  [[nodiscard]] constexpr auto operator<=>(const OperationCost& rhs) const
  {
    return std::tie(
               m_seek_count,
               m_storage_added_bytes,
               m_storage_replaced_bytes,
               m_storage_removed_bytes,
               m_storage_loaded_bytes,
               m_hash_node_calls
           ) <=>
        std::tie(
               rhs.m_seek_count,
               rhs.m_storage_added_bytes,
               rhs.m_storage_replaced_bytes,
               rhs.m_storage_removed_bytes,
               rhs.m_storage_loaded_bytes,
               rhs.m_hash_node_calls
        );
  }
};
/** @} */
} // namespace grovedb

#endif // LIBGROVEDB_COST_H

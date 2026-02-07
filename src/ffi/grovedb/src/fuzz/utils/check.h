// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#ifndef GROVEDB_FUZZ_UTILS_CHECK_H
#define GROVEDB_FUZZ_UTILS_CHECK_H

#include <grovedb/cost.h>

#include <cstdint>
#include <cstdlib>

/**
 * @brief Abort if expression is false.
 *
 * Used in fuzz targets where a failed invariant indicates a real bug
 * (not just an expected error return).
 */
#define CHECK_TRUE(expr)                                                                           \
  do {                                                                                             \
    if (!(expr)) {                                                                                 \
      __builtin_trap();                                                                            \
    }                                                                                              \
  } while (0)

/**
 * @brief Abort if a Result does not hold a value.
 *
 * Suitable for Result<T, E> values where the operation is expected to succeed.
 */
#define CHECK_OK(result) CHECK_TRUE((result).has_value())

/**
 * @brief Abort if two values are not equal.
 */
#define CHECK_EQ(a, b) CHECK_TRUE((a) == (b))

/** Sanity-check an OperationCost for overflow/corruption. */
inline void CheckCostSanity(const grovedb::OperationCost& cost)
{
  static constexpr uint64_t CEILING = 1'000'000'000;
  CHECK_TRUE(cost.m_seek_count < CEILING);
  CHECK_TRUE(cost.m_storage_added_bytes < CEILING);
  CHECK_TRUE(cost.m_storage_replaced_bytes < CEILING);
  CHECK_TRUE(cost.m_storage_removed_bytes < CEILING);
  CHECK_TRUE(cost.m_storage_loaded_bytes < CEILING);
  CHECK_TRUE(cost.m_hash_node_calls < CEILING);
}

/** Sanity-check the cost inside a Costed<T>. Usable with .transform(). */
inline constexpr auto CheckCost = [](const auto& costed) {
  CheckCostSanity(costed.cost());
};

#endif // GROVEDB_FUZZ_UTILS_CHECK_H

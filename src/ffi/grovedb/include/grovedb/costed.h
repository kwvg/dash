// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#ifndef LIBGROVEDB_COSTED_H
#define LIBGROVEDB_COSTED_H

#include <grovedb/cost.h>

#include <utility>

namespace grovedb {
/** @addtogroup cost
 *  @{ */
/**
 * Bundles a value with its associated OperationCost.
 *
 * Used as the success type in Result<Costed<T>, Error> for operations
 * that track resource consumption.
 */
template <typename T>
struct Costed {
  T m_value;
  OperationCost m_cost;

  /** Access the inner value. */
  [[nodiscard]] const T& value() const&
  {
    return m_value;
  }
  [[nodiscard]] T& value() &
  {
    return m_value;
  }
  [[nodiscard]] T&& value() &&
  {
    return std::move(m_value);
  }

  /** Access the operation cost. */
  [[nodiscard]] const OperationCost& cost() const
  {
    return m_cost;
  }
};
/** @} */
} // namespace grovedb

#endif // LIBGROVEDB_COSTED_H

// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#ifndef LIBGROVEDB_RESULT_H
#define LIBGROVEDB_RESULT_H

#include <grovedb/vendor/tl/expected.hpp>

namespace grovedb {
/** @addtogroup errors
 *  @{ */
template <class T, class E>
using Result = tl::expected<T, E>;

template <typename E>
auto Err(E&& e)
{
  return tl::unexpected(std::forward<E>(e));
}
/** @} */
} // namespace grovedb

#endif // LIBGROVEDB_RESULT_H

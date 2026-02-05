// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LIBGROVEDB_RESULT_H
#define LIBGROVEDB_RESULT_H

#include <grovedb/vendor/tl/expected.hpp>

namespace grovedb {
template <class T, class E>
using Result = tl::expected<T, E>;

template<typename E>
auto Err(E&& e) {
    return tl::unexpected(std::forward<E>(e));
}
} // namespace grovedb

#endif // LIBGROVEDB_RESULT_H

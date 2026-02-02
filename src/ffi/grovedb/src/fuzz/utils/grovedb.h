// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// FuzzedDataProvider helpers for GroveDB types.

#ifndef GROVEDB_FUZZ_UTILS_GROVEDB_H
#define GROVEDB_FUZZ_UTILS_GROVEDB_H

#include <FuzzedDataProvider.h>

#include <grovedb/element.h>
#include <grovedb/types.h>

#include <cstddef>
#include <cstdint>
#include <utility>

namespace grovedb {
namespace fuzz {

inline constexpr size_t MAX_KEY_LEN{64};
inline constexpr size_t MAX_VAL_LEN{256};
inline constexpr size_t MAX_DEPTH{8};

inline Bytes ConsumeKey(FuzzedDataProvider& fdp)
{
    auto len = fdp.ConsumeIntegralInRange<size_t>(1, MAX_KEY_LEN);
    return fdp.ConsumeBytes<uint8_t>(len);
}

inline Bytes ConsumeValue(FuzzedDataProvider& fdp)
{
    auto len = fdp.ConsumeIntegralInRange<size_t>(0, MAX_VAL_LEN);
    return fdp.ConsumeBytes<uint8_t>(len);
}

inline Path ConsumePath(FuzzedDataProvider& fdp)
{
    Path p;
    auto depth = fdp.ConsumeIntegralInRange<size_t>(0, MAX_DEPTH);
    for (size_t i{0}; i < depth; ++i) {
        p.push_back(ConsumeKey(fdp));
    }
    return p;
}

inline std::pair<Element, bool> ConsumeElement(FuzzedDataProvider& fdp)
{
    Element e;
    switch (fdp.ConsumeIntegralInRange(0, 3)) {
    case 0: return {e, Element::Item(ConsumeValue(fdp), e).ok()};
    case 1: return {e, Element::EmptyTree(e).ok()};
    case 2: return {e, Element::EmptySumTree(e).ok()};
    case 3: return {e, Element::SumItem(fdp.ConsumeIntegral<int64_t>(), e).ok()};
    }
    return {e, false};
}

} // namespace fuzz
} // namespace grovedb

#endif // GROVEDB_FUZZ_UTILS_GROVEDB_H

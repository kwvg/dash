// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#ifndef GROVEDB_FUZZ_UTILS_GROVEDB_H
#define GROVEDB_FUZZ_UTILS_GROVEDB_H

#include <FuzzedDataProvider.h>

#include <grovedb/element.h>
#include <grovedb/types.h>

#include <cstddef>
#include <cstdint>

namespace grovedb {
namespace fuzz {
/** Maximum byte length for a fuzzed key or value. */
static constexpr size_t MAX_BLOB_SIZE = 256;

/** Maximum number of segments in a fuzzed path. */
static constexpr size_t MAX_PATH_SEGMENTS = 8;

/**
 * Consume a key from the fuzz input (0..MAX_BLOB_SIZE bytes).
 *
 * @param[in,out] fdp  Fuzzed data provider.
 * @return Random key bytes.
 */
inline Bytes ConsumeKey(FuzzedDataProvider& fdp)
{
  auto len = fdp.ConsumeIntegralInRange<size_t>(0, MAX_BLOB_SIZE);
  auto vec = fdp.ConsumeBytes<uint8_t>(len);
  return Bytes{vec.begin(), vec.end()};
}

/**
 * Consume a value from the fuzz input (0..MAX_BLOB_SIZE bytes).
 *
 * @param[in,out] fdp  Fuzzed data provider.
 * @return Random value bytes.
 */
inline Bytes ConsumeValue(FuzzedDataProvider& fdp)
{
  return ConsumeKey(fdp);
}

/**
 * Consume a path (vector of byte segments) from the fuzz input.
 *
 * @param[in,out] fdp  Fuzzed data provider.
 * @return Random path with 0..MAX_PATH_SEGMENTS segments.
 */
inline Path ConsumePath(FuzzedDataProvider& fdp)
{
  auto count = fdp.ConsumeIntegralInRange<size_t>(0, MAX_PATH_SEGMENTS);
  Path path;
  path.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    path.push_back(ConsumeKey(fdp));
  }
  return path;
}

/**
 * Consume a random element from the fuzz input.
 *
 * Picks one of the four element factory methods at random and feeds it
 * fuzzed arguments. Returns an empty Element if construction fails.
 *
 * @param[in,out] fdp  Fuzzed data provider.
 * @return A randomly constructed Element (may be empty on factory error).
 */
inline Element ConsumeElement(FuzzedDataProvider& fdp)
{
  auto kind = fdp.ConsumeIntegralInRange<uint8_t>(0, 3);
  switch (kind) {
  case 0: {
    auto val = ConsumeValue(fdp);
    auto result = Element::Item(val);
    return result.has_value() ? std::move(*result) : Element{};
  }
  case 1: {
    auto result = Element::EmptyTree();
    return result.has_value() ? std::move(*result) : Element{};
  }
  case 2: {
    auto result = Element::EmptySumTree();
    return result.has_value() ? std::move(*result) : Element{};
  }
  case 3: {
    auto val = fdp.ConsumeIntegral<int64_t>();
    auto result = Element::SumItem(val);
    return result.has_value() ? std::move(*result) : Element{};
  }
  default:
    return Element{};
  }
}
} // namespace fuzz
} // namespace grovedb

#endif // GROVEDB_FUZZ_UTILS_GROVEDB_H

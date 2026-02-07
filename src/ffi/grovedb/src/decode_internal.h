// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#ifndef GROVEDB_DECODE_INTERNAL_H
#define GROVEDB_DECODE_INTERNAL_H

#include <grovedb/element.h>
#include <grovedb/query.h>
#include <grovedb/result.h>
#include <grovedb/wire.h>

#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace grovedb {
/**
 * Decode a wire-encoded list of PathKeyOptionalElementTrio entries.
 *
 * Wire format: `[u32 count][entry₁][entry₂]…`
 *
 * Each entry:
 *   `[path (wire-encoded)][key (u32 len + bytes)][u8 has_element]`
 *   If has_element == 1: `[u32 elem_len][elem bincode]`
 *
 * @param[in] data  The wire-encoded bytes.
 * @return Decoded entries, or a wire error.
 */
inline Result<std::vector<PathKeyElement>, wire::Error>
DecodePathKeyElements(std::span<const uint8_t> data)
{
  wire::Reader r{data};
  auto count_r = r.U32();
  if (!count_r) {
    return Err(count_r.error());
  }
  uint32_t count = *count_r;
  if (count > wire::MAX_VECTOR_SIZE) {
    return Err(wire::Error::InvalidArgument);
  }

  std::vector<PathKeyElement> results;
  results.reserve(count);
  for (uint32_t i{0}; i < count; ++i) {
    PathKeyElement entry;
    auto path_r = wire::Read<Path>(r);
    if (!path_r) {
      return Err(path_r.error());
    }
    entry.m_path = std::move(*path_r);

    auto key_r = r.Bytes();
    if (!key_r) {
      return Err(key_r.error());
    }
    entry.m_key = std::move(*key_r);

    auto has_r = r.U8();
    if (!has_r) {
      return Err(has_r.error());
    }
    if (*has_r) {
      auto elem_r = r.Bytes();
      if (!elem_r) {
        return Err(elem_r.error());
      }
      entry.m_element = Element::FromData(std::move(*elem_r));
    }

    results.push_back(std::move(entry));
  }
  return results;
}
} // namespace grovedb

#endif // GROVEDB_DECODE_INTERNAL_H

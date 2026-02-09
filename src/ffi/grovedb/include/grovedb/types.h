// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#ifndef LIBGROVEDB_TYPES_H
#define LIBGROVEDB_TYPES_H

#include <grovedb/error.h>
#include <grovedb/result.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace grovedb {
/** @addtogroup types
 *  @{ */
/** A byte vector. */
struct Bytes : std::vector<uint8_t> {
  using std::vector<uint8_t>::vector;

  /** Implicit conversion from std::vector<uint8_t>. */
  Bytes(std::vector<uint8_t> v)
      : std::vector<uint8_t>(std::move(v))
  {
  } // NOLINT(google-explicit-constructor)

  /**
   * Create from raw string bytes (reinterprets chars as uint8_t).
   * @param[in] sv  String to convert.
   * @return The constructed byte vector.
   */
  [[nodiscard]] static Bytes FromString(std::string_view sv)
  {
    return Bytes(sv.begin(), sv.end());
  }

  /**
   * Create by parsing a hex string ("48656c6c6f" -> {0x48,0x65,...}).
   * @param[in] sv  Hex string (2 chars per byte, case-insensitive).
   * @return The parsed byte vector, or an error.
   */
  [[nodiscard]] static Result<Bytes, Error> FromHex(std::string_view sv);

  /**
   * Return raw string (reinterprets bytes as chars).
   * @return String from raw bytes.
   */
  [[nodiscard]] std::string ToString() const
  {
    return std::string(begin(), end());
  }

  /**
   * Return lowercase hex representation ("48656c6c6f").
   * @return Hex string.
   */
  [[nodiscard]] std::string ToHex() const;
};

/** A path, represented as a vector of byte vectors. */
using Path = std::vector<Bytes>;

/** A 32-byte hash (Merkle root, etc.). */
struct Hash : std::array<uint8_t, 32> {
  /**
   * Return hex representation (64 chars).
   * @return Hex string.
   */
  [[nodiscard]] std::string ToString() const;

  /**
   * Create by parsing a 64-char hex string.
   * @param[in] sv  Hex string (exactly 64 characters).
   * @return The parsed hash, or an error.
   */
  [[nodiscard]] static Result<Hash, Error> FromHex(std::string_view sv);
};
/** @} */
} // namespace grovedb

#endif // LIBGROVEDB_TYPES_H

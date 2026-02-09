// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#ifndef LIBGROVEDB_WIRE_H
#define LIBGROVEDB_WIRE_H

#include <grovedb/result.h>
#include <grovedb/types.h>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

namespace grovedb {
/** Little-endian binary serialization for on-disk and network encoding. */
namespace wire {
/** @addtogroup wire
 *  @{ */
/** Maximum number of segments in a wire-encoded path. */
inline constexpr uint32_t MAX_PATH_SEGMENTS = 256;

/** Maximum number of elements in a wire-encoded vector. */
inline constexpr uint32_t MAX_VECTOR_SIZE = 1000000;

/** Wire-level error codes for serialization and deserialization. */
enum class Error {
  /** Buffer underflow or malformed data. */
  Corruption,
  /** Invalid argument or constraint violation. */
  InvalidArgument,
};

/**
 * Sequential reader for little-endian wire format.
 *
 * Reads from a borrowed byte span with an advancing cursor.
 * Every read returns Result; callers must check before using the value.
 */
class Reader
{
public:
  /**
   * Construct a reader over a byte span.
   *
   * @param[in] data  The wire bytes to read from (borrowed, not copied).
   */
  explicit Reader(std::span<const uint8_t> data);

  /** @name Primitive reads (LE to native) */
  ///@{

  /**
   * Read an unsigned 8-bit integer.
   *
   * @return The decoded value on success; Error::Corruption on buffer underflow.
   */
  [[nodiscard]] Result<uint8_t, Error> U8();

  /**
   * Read an unsigned 16-bit integer from little-endian.
   *
   * @return The decoded value on success; Error::Corruption on buffer underflow.
   */
  [[nodiscard]] Result<uint16_t, Error> U16();

  /**
   * Read an unsigned 32-bit integer from little-endian.
   *
   * @return The decoded value on success; Error::Corruption on buffer underflow.
   */
  [[nodiscard]] Result<uint32_t, Error> U32();

  /**
   * Read an unsigned 64-bit integer from little-endian.
   *
   * @return The decoded value on success; Error::Corruption on buffer underflow.
   */
  [[nodiscard]] Result<uint64_t, Error> U64();

  ///@}

  /**
   * Read exactly `out.size()` raw bytes.
   *
   * @param[out] out  Span to fill with the next bytes from the buffer.
   * @return Success (void) or Error::Corruption on buffer underflow.
   */
  [[nodiscard]] Result<void, Error> Raw(std::span<uint8_t> out);

  /**
   * Read length-prefixed bytes (u32 length + data).
   *
   * @return The decoded byte vector on success; an error otherwise.
   */
  [[nodiscard]] Result<Bytes, Error> Bytes();

  /**
   * @return Number of unread bytes remaining.
   */
  [[nodiscard]] size_t remaining() const;

  /**
   * @return True if the cursor has consumed all input.
   */
  [[nodiscard]] bool at_end() const;

private:
  std::span<const uint8_t> m_data;
  size_t m_pos{0};
};

/**
 * Sequential writer for little-endian wire format.
 *
 * Appends to an internal byte buffer. Write operations are infallible but
 * allocation failure is catastrophic (throws std::bad_alloc).
 */
class Writer
{
public:
  Writer() = default;

  /**
   * Construct a writer with a size hint for pre-allocation.
   *
   * @param[in] reserve_hint  Number of bytes to pre-allocate.
   */
  explicit Writer(size_t reserve_hint);

  /** @name Primitive writes (native to LE) */
  ///@{

  /**
   * Write an unsigned 8-bit integer.
   *
   * @param[in] v  Value to write.
   */
  void U8(uint8_t v);

  /**
   * Write an unsigned 16-bit integer in little-endian.
   *
   * @param[in] v  Value to write.
   */
  void U16(uint16_t v);

  /**
   * Write an unsigned 32-bit integer in little-endian.
   *
   * @param[in] v  Value to write.
   */
  void U32(uint32_t v);

  /**
   * Write an unsigned 64-bit integer in little-endian.
   *
   * @param[in] v  Value to write.
   */
  void U64(uint64_t v);

  ///@}

  /**
   * Write raw bytes without a length prefix.
   *
   * @param[in] data  Bytes to append.
   */
  void Raw(std::span<const uint8_t> data);

  /**
   * Write length-prefixed bytes (u32 length + data).
   *
   * @param[in] data  Bytes to write, preceded by their u32 length.
   */
  void Bytes(std::span<const uint8_t> data);

  /**
   * @return Read-only view of the buffer written so far.
   */
  [[nodiscard]] std::span<const uint8_t> data() const;

  /**
   * Move the internal buffer out. Writer is left empty.
   *
   * @return The accumulated byte buffer.
   */
  [[nodiscard]] std::vector<uint8_t> Take();

private:
  std::vector<uint8_t> m_buf;
};

// =========================================================================
// ADL free functions for wire encoding/decoding
// =========================================================================

/**
 * Encode a byte vector as length-prefixed bytes.
 *
 * @param[in] w  Writer to append to.
 * @param[in] v  Byte vector to encode.
 */
inline void Write(Writer& w, const grovedb::Bytes& v)
{
  w.Bytes(v);
}

/**
 * Decode a byte vector from length-prefixed bytes.
 *
 * @return The decoded byte vector on success; an error otherwise.
 */
template <typename T>
  requires std::same_as<T, grovedb::Bytes>
Result<T, Error> Read(Reader& r)
{
  return r.Bytes();
}

/**
 * Encode a Path (vector of byte vectors) to wire format.
 *
 * Wire layout: `[u32 count][Bytes₁][Bytes₂]…`
 *
 * @param[in] w     Writer to append to.
 * @param[in] path  Path to encode.
 */
inline void Write(Writer& w, const Path& path)
{
  w.U32(static_cast<uint32_t>(path.size()));
  for (const auto& segment : path) {
    w.Bytes(segment);
  }
}

/**
 * Decode a Path from wire format.
 *
 * Wire layout: `[u32 count][Bytes₁][Bytes₂]…`
 *
 * @return The decoded path on success; an error otherwise.
 */
template <typename T>
  requires std::same_as<T, Path>
Result<T, Error> Read(Reader& r)
{
  auto count_result = r.U32();
  if (!count_result) {
    return Err(count_result.error());
  }
  uint32_t count = *count_result;

  if (count > MAX_PATH_SEGMENTS) {
    return Err(Error::InvalidArgument);
  }

  Path path;
  path.reserve(count);
  for (uint32_t i{0}; i < count; ++i) {
    auto segment_result = r.Bytes();
    if (!segment_result) {
      return Err(segment_result.error());
    }
    path.push_back(std::move(*segment_result));
  }
  return path;
}

/**
 * Encode a vector of T using the ADL Write overload for T.
 *
 * Wire layout: `[u32 count][T₁][T₂]…`
 *
 * @param[in] w  Writer to append to.
 * @param[in] v  Vector to encode.
 */
template <typename T>
void Write(Writer& w, const std::vector<T>& v)
{
  w.U32(static_cast<uint32_t>(v.size()));
  for (const auto& item : v) {
    Write(w, item);
  }
}

/**
 * Decode a vector of T using the ADL Read overload for T.
 *
 * Wire layout: `[u32 count][T₁][T₂]…`
 *
 * @return The decoded vector on success; an error otherwise.
 */
template <typename T>
  requires(!std::same_as<T, grovedb::Bytes> && !std::same_as<T, Path>)
Result<std::vector<T>, Error> Read(Reader& r)
{
  auto count_result = r.U32();
  if (!count_result) {
    return Err(count_result.error());
  }
  uint32_t count = *count_result;

  if (count > MAX_VECTOR_SIZE) {
    return Err(Error::InvalidArgument);
  }

  std::vector<T> vec;
  vec.reserve(count);
  for (uint32_t i{0}; i < count; ++i) {
    auto item_result = Read<T>(r);
    if (!item_result) {
      return Err(item_result.error());
    }
    vec.push_back(std::move(*item_result));
  }
  return vec;
}

/**
 * Encode a value to a byte vector.
 *
 * @param[in] v  The value to encode.
 * @return The encoded bytes.
 */
template <typename T>
std::vector<uint8_t> Encode(const T& v)
{
  Writer w;
  Write(w, v);
  return w.Take();
}

/**
 * Decode a value from a byte span.
 *
 * @param[in] data  Raw wire bytes.
 * @return The decoded value on success; an error otherwise.
 */
template <typename T>
Result<T, Error> Decode(std::span<const uint8_t> data)
{
  Reader r{data};
  return Read<T>(r);
}
/** @return String representation of the wire error. */
constexpr std::string_view ToString(Error err)
{
  switch (err) {
  case Error::Corruption:
    return "Corruption";
  case Error::InvalidArgument:
    return "InvalidArgument";
  } // no default case, so the compiler can warn about missing cases
  return "Unknown";
}

/** @brief Stream insertion for wire::Error. */
inline std::ostream& operator<<(std::ostream& os, Error err)
{
  return os << ToString(err);
}
/** @} */
} // namespace wire
} // namespace grovedb

#endif // LIBGROVEDB_WIRE_H

// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#ifndef GROVEDB_UTIL_FFI_H
#define GROVEDB_UTIL_FFI_H

#include <grovedb/costed.h>
#include <grovedb/db.h>
#include <grovedb/element.h>

#include <rust/grovedb_cxx/lib.h>

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace grovedb {
// ---------------------------------------------------------------------------
// Low-level FFI conversion helpers
// ---------------------------------------------------------------------------

/** Convert an FFI operation cost struct to the public C++ type. */
inline OperationCost convert_cost(const grovedb_cxx::FfiOperationCost& ffi)
{
  return OperationCost{
      .m_seek_count = ffi.seek_count,
      .m_storage_added_bytes = ffi.storage_added_bytes,
      .m_storage_replaced_bytes = ffi.storage_replaced_bytes,
      .m_storage_removed_bytes = ffi.storage_removed_bytes,
      .m_storage_loaded_bytes = ffi.storage_loaded_bytes,
      .m_hash_node_calls = ffi.hash_node_calls,
  };
}

/** Classify an exception message into the most appropriate Error type. */
inline Error StringToError(const char* what)
{
  std::string_view msg{what};
  if (msg.find("not found") != std::string_view::npos) {
    return Error::NotFound(std::string(msg));
  }
  if (msg.find("corrupt") != std::string_view::npos) {
    return Error::Corruption(std::string(msg));
  }
  if (msg.find("invalid") != std::string_view::npos) {
    return Error::InvalidArgument(std::string(msg));
  }
  return Error::IOError(std::string(msg));
}

// ---------------------------------------------------------------------------
// High-level FFI helpers
// ---------------------------------------------------------------------------

/** Wrap an FFI-calling lambda in try-catch, converting exceptions to Error. */
template <typename F>
auto CallFFI(F&& fn) -> std::invoke_result_t<F>
{
  try {
    return std::forward<F>(fn)();
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

/** Create a rust::Slice from any contiguous byte range. */
inline rust::Slice<const uint8_t> ToSlice(std::span<const uint8_t> s)
{
  return {s.data(), s.size()};
}

/** Convert FfiElementResult -> Costed<Element>. */
inline Costed<Element> ConvertElement(const grovedb_cxx::FfiElementResult& result)
{
  return {
      Element::FromData(Bytes{result.element.begin(), result.element.end()}),
      convert_cost(result.cost)
  };
}

/** Convert FfiBoolResult -> Costed<bool>. */
inline Costed<bool> ConvertBool(const grovedb_cxx::FfiBoolResult& result)
{
  return {result.value, convert_cost(result.cost)};
}

/** Convert FfiU32Result -> Costed<uint32_t>. */
inline Costed<uint32_t> ConvertU32(const grovedb_cxx::FfiU32Result& result)
{
  return {result.value, convert_cost(result.cost)};
}

/** Convert FfiOptionalElementResult -> Costed<std::optional<Element>>. */
inline Costed<std::optional<Element>>
ConvertOptionalElement(const grovedb_cxx::FfiOptionalElementResult& result)
{
  auto cost = convert_cost(result.cost);
  if (result.has_element) {
    return {Element::FromData(Bytes{result.element.begin(), result.element.end()}), cost};
  }
  return {std::nullopt, cost};
}

/** Convert FfiChangedValueResult -> Costed<Db::ChangedValue>. */
inline Costed<Db::ChangedValue>
ConvertChangedValue(const grovedb_cxx::FfiChangedValueResult& result)
{
  auto cost = convert_cost(result.cost);
  std::optional<Element> prev;
  if (result.has_previous_element) {
    prev = Element::FromData(Bytes{result.previous_element.begin(), result.previous_element.end()});
  }
  return {Db::ChangedValue{result.changed, std::move(prev)}, cost};
}

/** Convert FfiOptionalBytesResult -> Costed<std::optional<Bytes>>. */
inline Costed<std::optional<Bytes>>
ConvertOptionalBytes(const grovedb_cxx::FfiOptionalBytesResult& result)
{
  auto cost = convert_cost(result.cost);
  if (result.has_value) {
    return {Bytes{result.value.begin(), result.value.end()}, cost};
  }
  return {std::nullopt, cost};
}

/** Validate and copy a 32-byte hash from an FFI result. */
inline Result<Hash, Error> HashFromSlice(const rust::Vec<uint8_t>& data)
{
  if (data.size() != 32) {
    return Err(
        Error::Corruption("root hash: expected 32 bytes, got " + std::to_string(data.size()))
    );
  }
  Hash hash{};
  std::copy(data.begin(), data.end(), hash.begin());
  return hash;
}
} // namespace grovedb

#endif // GROVEDB_UTIL_FFI_H

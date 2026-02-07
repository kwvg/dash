// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#ifndef LIBGROVEDB_ELEMENT_H
#define LIBGROVEDB_ELEMENT_H

#include <grovedb/error.h>
#include <grovedb/result.h>
#include <grovedb/types.h>

#include <cstdint>
#include <iostream>
#include <string_view>

namespace grovedb {
class Db;

/**
 * Element type discriminant.
 */
enum class ElementType : uint8_t {
  Item = 0,
  Tree,
  SumTree,
  SumItem,
};

/** @return String representation of the element type. */
constexpr std::string_view ToString(ElementType type)
{
  switch (type) {
  case ElementType::Item:
    return "Item";
  case ElementType::Tree:
    return "Tree";
  case ElementType::SumTree:
    return "SumTree";
  case ElementType::SumItem:
    return "SumItem";
  } // no default case, so the compiler can warn about missing cases
  return "Unknown";
}

/**
 * Opaque container for a GroveDB element in its serialized (bincode) form.
 *
 * Populated by Db::Get / Db::GetDirect / Db::GetOptional, or constructed
 * with one of the static factory methods. The raw bytes can be inspected
 * via data() and are accepted by Db::Put and related methods.
 */
class Element
{
public:
  Element() = default;

  // -- Factories ----------------------------------------------------------

  /**
   * Create an item element containing arbitrary value bytes.
   *
   * @param[in] value  Raw value payload.
   * @return The constructed element, or an error.
   */
  [[nodiscard]] static Result<Element, Error> Item(const Bytes& value);

  /**
   * Create an empty subtree element.
   *
   * @return The constructed element, or an error.
   */
  [[nodiscard]] static Result<Element, Error> EmptyTree();

  /**
   * Create an empty sum tree element.
   *
   * @return The constructed element, or an error.
   */
  [[nodiscard]] static Result<Element, Error> EmptySumTree();

  /**
   * Create a sum item element with the given value.
   *
   * @param[in] value  The sum value.
   * @return The constructed element, or an error.
   */
  [[nodiscard]] static Result<Element, Error> SumItem(int64_t value);

  // -- Accessors ----------------------------------------------------------

  /** Access the raw serialized representation. */
  [[nodiscard]] const Bytes& data() const
  {
    return m_data;
  }

  /** True when this element holds no data (default-constructed). */
  [[nodiscard]] bool empty() const
  {
    return m_data.empty();
  }

  /** Equality based on the raw serialized representation. */
  [[nodiscard]] bool operator==(const Element& rhs) const
  {
    return m_data == rhs.m_data;
  }

  /**
   * Construct an Element directly from raw serialized (bincode) bytes.
   *
   * This is used internally to reconstitute elements from FFI results.
   *
   * @param[in] data  The raw bincode bytes.
   * @return The constructed element.
   */
  [[nodiscard]] static Element FromData(Bytes data)
  {
    Element e;
    e.m_data = std::move(data);
    return e;
  }

private:
  friend class Db;
  Bytes m_data;
};
/** @brief Stream insertion for ElementType. */
inline std::ostream& operator<<(std::ostream& os, ElementType type)
{
  return os << ToString(type);
}
} // namespace grovedb

#endif // LIBGROVEDB_ELEMENT_H

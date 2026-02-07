// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#ifndef LIBGROVEDB_QUERY_H
#define LIBGROVEDB_QUERY_H

#include <grovedb/element.h>
#include <grovedb/result.h>
#include <grovedb/types.h>
#include <grovedb/wire.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace grovedb {
class Db;

/**
 * Descriptor for a single query predicate within a PathQuery.
 *
 * Use the static factory methods to create instances.
 */
class QueryItem
{
public:
  /** @name Factories */
  ///@{

  /** Match a single exact key. */
  static QueryItem Key(const Bytes& key);
  /** Match keys in [start, end). */
  static QueryItem Range(const Bytes& start, const Bytes& end);
  /** Match keys in [start, end]. */
  static QueryItem RangeInclusive(const Bytes& start, const Bytes& end);
  /** Match all keys. */
  static QueryItem RangeFull();
  /** Match keys in [start, +inf). */
  static QueryItem RangeFrom(const Bytes& start);
  /** Match keys in (-inf, end). */
  static QueryItem RangeTo(const Bytes& end);
  /** Match keys in (-inf, end]. */
  static QueryItem RangeToInclusive(const Bytes& end);
  /** Match keys strictly after `after` (exclusive start). */
  static QueryItem RangeAfter(const Bytes& after);
  /** Match keys in (after, to). */
  static QueryItem RangeAfterTo(const Bytes& after, const Bytes& to);
  /** Match keys in (after, to]. */
  static QueryItem RangeAfterToInclusive(const Bytes& after, const Bytes& to);

  ///@}

  /** Query item kind (wire format tag). */
  uint8_t kind() const
  {
    return m_kind;
  }
  /** First operand (key / start / after). */
  const Bytes& first() const
  {
    return m_a;
  }
  /** Second operand (end / to), empty for single-operand items. */
  const Bytes& second() const
  {
    return m_b;
  }

  /**
   * Serialize this item to a wire::Writer.
   *
   * Wire layout: `[u8 kind][kind-specific length-prefixed fields]`.
   *
   * @param[in] w  Writer to append to.
   */
  void Encode(wire::Writer& w) const;

  /**
   * Deserialize a QueryItem from a wire::Reader.
   *
   * @param[in] r  Reader to consume from.
   * @return The decoded QueryItem, or a wire error.
   */
  [[nodiscard]] static Result<QueryItem, wire::Error> Decode(wire::Reader& r);

  /** Default-construct an empty QueryItem (kind 0, no data). */
  QueryItem() = default;

private:
  friend class PathQuery;

  QueryItem(uint8_t kind, Bytes a, Bytes b)
      : m_kind{kind}
      , m_a{std::move(a)}
      , m_b{std::move(b)}
  {
  }

  uint8_t m_kind{0};
  Bytes m_a;
  Bytes m_b;
};

/**
 * Opaque handle for a GroveDB path query.
 *
 * Created via static factory methods. Passed to Db query methods to
 * execute the query.
 */
class PathQuery
{
public:
  PathQuery();
  ~PathQuery();

  PathQuery(const PathQuery&) = delete;
  PathQuery& operator=(const PathQuery&) = delete;
  PathQuery(PathQuery&&);
  PathQuery& operator=(PathQuery&&);

  /**
   * Create a PathQuery.
   *
   * @param[in] path   Path to the starting subtree.
   * @param[in] items  Query predicates to apply.
   * @param[in] limit  Maximum number of results (0 = no limit).
   * @param[in] offset Number of results to skip (0 = none).
   * @return The constructed PathQuery, or an error.
   */
  [[nodiscard]] static Result<PathQuery, Error> New(
      const Path& path, const std::vector<QueryItem>& items, uint32_t limit = 0, uint32_t offset = 0
  );

  /**
   * Create a PathQuery with a default subquery branch.
   *
   * The subquery is applied to every element returned by the outer
   * query, enabling hierarchical (two-level) queries.
   *
   * @param[in] path            Path to the starting subtree.
   * @param[in] items           Outer query predicates.
   * @param[in] limit           Maximum number of results (0 = no limit).
   * @param[in] offset          Number of results to skip (0 = none).
   * @param[in] subquery_path   Path segments for subquery navigation
   *                            (empty means no subquery path).
   * @param[in] subquery_items  Subquery predicates (empty means no
   *                            subquery).
   * @return The constructed PathQuery, or an error.
   */
  [[nodiscard]] static Result<PathQuery, Error> NewWithSubquery(
      const Path& path,
      const std::vector<QueryItem>& items,
      uint32_t limit,
      uint32_t offset,
      const Path& subquery_path,
      const std::vector<QueryItem>& subquery_items
  );

private:
  friend class Db;
  struct Impl;
  std::unique_ptr<Impl> m_impl;
};

// ---------------------------------------------------------------------------
// Query result types
// ---------------------------------------------------------------------------

/** Kind tag for QueryItemOrSum entries. */
enum class QueryItemOrSumKind : uint8_t {
  ItemData = 0,
  SumValue = 1,
  BigSumValue = 2,
  CountValue = 3,
  CountSumValue = 4,
  ItemDataWithSum = 5,
};

/** @return String representation of the kind. */
constexpr std::string_view ToString(QueryItemOrSumKind kind)
{
  switch (kind) {
  case QueryItemOrSumKind::ItemData:
    return "ItemData";
  case QueryItemOrSumKind::SumValue:
    return "SumValue";
  case QueryItemOrSumKind::BigSumValue:
    return "BigSumValue";
  case QueryItemOrSumKind::CountValue:
    return "CountValue";
  case QueryItemOrSumKind::CountSumValue:
    return "CountSumValue";
  case QueryItemOrSumKind::ItemDataWithSum:
    return "ItemDataWithSum";
  } // no default case, so the compiler can warn about missing cases
  return "Unknown";
}

/**
 * Tagged union entry from query_item_value_or_sum results.
 *
 * Check `m_kind` to determine which data members are populated.
 */
struct QueryItemOrSum {
  QueryItemOrSumKind m_kind{QueryItemOrSumKind::ItemData};
  Bytes m_item_data; /**< For ItemData, ItemDataWithSum. */
  int64_t m_sum_value{0}; /**< For SumValue, CountSumValue, ItemDataWithSum. */
  int64_t m_big_sum_hi{0}; /**< High 8 bytes of BigSumValue (i128 as two i64). */
  int64_t m_big_sum_lo{0}; /**< Low 8 bytes of BigSumValue. */
  uint64_t m_count_value{0}; /**< For CountValue, CountSumValue. */
};

/** Kind tag for QueryResultElement entries. */
enum class QueryResultKind : uint8_t {
  Element = 0,
  KeyElementPair = 1,
  PathKeyElementTrio = 2,
};

/** @return String representation of the kind. */
constexpr std::string_view ToString(QueryResultKind kind)
{
  switch (kind) {
  case QueryResultKind::Element:
    return "Element";
  case QueryResultKind::KeyElementPair:
    return "KeyElementPair";
  case QueryResultKind::PathKeyElementTrio:
    return "PathKeyElementTrio";
  } // no default case, so the compiler can warn about missing cases
  return "Unknown";
}

/**
 * Single result element from query_raw / query_many_raw.
 *
 * The `m_kind` field determines which members are populated.
 */
struct QueryResultElement {
  QueryResultKind m_kind{QueryResultKind::Element};
  grovedb::Element m_element;
  Bytes m_key; /**< For KeyElementPair, PathKeyElementTrio. */
  Path m_path; /**< For PathKeyElementTrio only. */
};

/**
 * Single entry from query_keys_optional / query_raw_keys_optional.
 *
 * Represents a path-key pair with an optional element (nullopt when the
 * key was not found).
 */
struct PathKeyElement {
  Path m_path;
  Bytes m_key;
  std::optional<grovedb::Element> m_element;
};

/**
 * Result bundle for queries that return values with a skip count.
 */
template <typename T>
struct QueryData {
  T m_data;
  uint16_t m_skipped{0};
};

// ---------------------------------------------------------------------------
// Wire ADL overloads for QueryItem
// ---------------------------------------------------------------------------

namespace wire {
/**
 * Encode a single QueryItem to a wire::Writer.
 *
 * @param[in] w     Writer to append to.
 * @param[in] item  QueryItem to encode.
 */
inline void Write(Writer& w, const QueryItem& item)
{
  item.Encode(w);
}

/**
 * Decode a single QueryItem from a wire::Reader.
 *
 * @param[in] r  Reader to consume from.
 * @return The decoded QueryItem, or a wire error.
 */
template <typename T>
  requires std::same_as<T, QueryItem>
Result<T, Error> Read(Reader& r)
{
  return QueryItem::Decode(r);
}
} // namespace wire

/** @brief Stream insertion for QueryItemOrSumKind. */
inline std::ostream& operator<<(std::ostream& os, QueryItemOrSumKind kind)
{
  return os << ToString(kind);
}

/** @brief Stream insertion for QueryResultKind. */
inline std::ostream& operator<<(std::ostream& os, QueryResultKind kind)
{
  return os << ToString(kind);
}
} // namespace grovedb

#endif // LIBGROVEDB_QUERY_H

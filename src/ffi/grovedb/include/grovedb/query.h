// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GROVEDB_QUERY_H
#define GROVEDB_QUERY_H

#include <grovedb/status.h>
#include <grovedb/types.h>

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace grovedb {
class Db;
namespace wire {
class Writer;
class Reader;
} // namespace wire

/**
 * Descriptor for a single query predicate within a PathQuery.
 *
 * Use the static factory methods to create instances.
 */
class QueryItem
{
public:
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

    /** Query item kind (wire format tag). */
    uint8_t kind() const { return m_kind; }
    /** First operand (key / start / after). */
    const Bytes& first() const { return m_a; }
    /** Second operand (end / to), empty for single-operand items. */
    const Bytes& second() const { return m_b; }

    /**
     * Serialize this item to a wire::Writer.
     *
     * Wire layout: [u8 kind][kind-specific length-prefixed fields].
     *
     * @param[in] w  Writer to append to.
     */
    void Encode(wire::Writer& w) const;

    /**
     * Deserialize a QueryItem from a wire::Reader.
     *
     * Reconstructs the item via the appropriate factory method.
     *
     * @param[in]  r     Reader to consume from.
     * @param[out] item  Receives the decoded QueryItem.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    static Status Decode(wire::Reader& r, QueryItem& item);

    /** Default-construct an empty QueryItem (kind 0, no data). */
    QueryItem() = default;

private:
    friend class PathQuery;

    QueryItem(uint8_t kind, Bytes a, Bytes b)
        : m_kind{kind}, m_a{std::move(a)}, m_b{std::move(b)} {}

    uint8_t m_kind{0};
    Bytes m_a;
    Bytes m_b;
};

/**
 * Opaque handle for a GroveDB path query.
 *
 * Created via static factory methods.  Passed to Db::QueryValues() to
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
     * @param[in]  path   Path to the starting subtree.
     * @param[in]  items  Query predicates to apply.
     * @param[in]  limit  Maximum number of results (0 = no limit).
     * @param[in]  offset Number of results to skip (0 = none).
     * @param[out] query  Receives the constructed PathQuery.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    static Status New(
        const Path& path,
        const std::vector<QueryItem>& items,
        uint32_t limit,
        uint32_t offset,
        PathQuery& query);

    /**
     * Create a PathQuery with a default subquery branch.
     *
     * The subquery is applied to every element returned by the outer
     * query, enabling hierarchical (two-level) queries.
     *
     * @param[in]  path            Path to the starting subtree.
     * @param[in]  items           Outer query predicates.
     * @param[in]  limit           Maximum number of results (0 = no limit).
     * @param[in]  offset          Number of results to skip (0 = none).
     * @param[in]  subquery_path   Path segments for subquery navigation
     *                             (empty means no subquery path).
     * @param[in]  subquery_items  Subquery predicates (empty means no
     *                             subquery).
     * @param[out] query           Receives the constructed PathQuery.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    static Status NewWithSubquery(
        const Path& path,
        const std::vector<QueryItem>& items,
        uint32_t limit,
        uint32_t offset,
        const Path& subquery_path,
        const std::vector<QueryItem>& subquery_items,
        PathQuery& query);

private:
    friend class Db;
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace grovedb

#endif // GROVEDB_QUERY_H

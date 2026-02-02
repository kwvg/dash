// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifdef HAVE_GROVEDB_CONFIG
#include <config/grovedb-config.h>
#endif

#include "db_internal.h"

#include <grovedb/query.h>
#include <grovedb/wire.h>

#include <util/assumptions.h>

#include <rust/grovedb_cxx/lib.h>
#include <types/query.h>
#include <types/transaction.h>

#include <util/assert.h>

#include <limits>
#include <stdexcept>

namespace grovedb {

// ---------------------------------------------------------------------------
// QueryItem serialization
// ---------------------------------------------------------------------------

void QueryItem::Encode(wire::Writer& w) const
{
    w.U8(m_kind);
    switch (m_kind) {
    case 0: // Key
        w.Bytes(m_a);
        break;
    case 1: // Range
    case 2: // RangeInclusive
    case 8: // RangeAfterTo
    case 9: // RangeAfterToInclusive
        w.Bytes(m_a);
        w.Bytes(m_b);
        break;
    case 3: // RangeFull
        break;
    case 4: // RangeFrom
    case 5: // RangeTo
    case 6: // RangeToInclusive
    case 7: // RangeAfter
        w.Bytes(m_a);
        break;
    default:
        Assert(false, "invalid QueryItem kind");
        throw std::invalid_argument("invalid QueryItem kind");
    }
}

Status QueryItem::Decode(wire::Reader& r, QueryItem& item)
{
    uint8_t kind{0};
    if (auto s = r.U8(kind); !s.ok()) return s;

    Bytes a;
    Bytes b;

    switch (kind) {
    case 0: // Key
        if (auto s = r.Bytes(a); !s.ok()) return s;
        item = QueryItem::Key(a);
        break;
    case 1: // Range
        if (auto s = r.Bytes(a); !s.ok()) return s;
        if (auto s = r.Bytes(b); !s.ok()) return s;
        item = QueryItem::Range(a, b);
        break;
    case 2: // RangeInclusive
        if (auto s = r.Bytes(a); !s.ok()) return s;
        if (auto s = r.Bytes(b); !s.ok()) return s;
        item = QueryItem::RangeInclusive(a, b);
        break;
    case 3: // RangeFull
        item = QueryItem::RangeFull();
        break;
    case 4: // RangeFrom
        if (auto s = r.Bytes(a); !s.ok()) return s;
        item = QueryItem::RangeFrom(a);
        break;
    case 5: // RangeTo
        if (auto s = r.Bytes(a); !s.ok()) return s;
        item = QueryItem::RangeTo(a);
        break;
    case 6: // RangeToInclusive
        if (auto s = r.Bytes(a); !s.ok()) return s;
        item = QueryItem::RangeToInclusive(a);
        break;
    case 7: // RangeAfter
        if (auto s = r.Bytes(a); !s.ok()) return s;
        item = QueryItem::RangeAfter(a);
        break;
    case 8: // RangeAfterTo
        if (auto s = r.Bytes(a); !s.ok()) return s;
        if (auto s = r.Bytes(b); !s.ok()) return s;
        item = QueryItem::RangeAfterTo(a, b);
        break;
    case 9: // RangeAfterToInclusive
        if (auto s = r.Bytes(a); !s.ok()) return s;
        if (auto s = r.Bytes(b); !s.ok()) return s;
        item = QueryItem::RangeAfterToInclusive(a, b);
        break;
    default:
        return Status::Corruption("wire: unknown QueryItem kind");
    }

    return Status::Ok();
}

// ---------------------------------------------------------------------------
// QueryItem factories
// ---------------------------------------------------------------------------

QueryItem QueryItem::Key(const Bytes& key) { return {0, key, {}}; }
QueryItem QueryItem::Range(const Bytes& start, const Bytes& end) { return {1, start, end}; }
QueryItem QueryItem::RangeInclusive(const Bytes& start, const Bytes& end) { return {2, start, end}; }
QueryItem QueryItem::RangeFull() { return {3, {}, {}}; }
QueryItem QueryItem::RangeFrom(const Bytes& start) { return {4, start, {}}; }
QueryItem QueryItem::RangeTo(const Bytes& end) { return {5, end, {}}; }
QueryItem QueryItem::RangeToInclusive(const Bytes& end) { return {6, end, {}}; }
QueryItem QueryItem::RangeAfter(const Bytes& after) { return {7, after, {}}; }
QueryItem QueryItem::RangeAfterTo(const Bytes& after, const Bytes& to) { return {8, after, to}; }
QueryItem QueryItem::RangeAfterToInclusive(const Bytes& after, const Bytes& to) { return {9, after, to}; }

// ---------------------------------------------------------------------------
// PathQuery
// ---------------------------------------------------------------------------

PathQuery::PathQuery() = default;
PathQuery::~PathQuery() = default;
PathQuery::PathQuery(PathQuery&&) = default;
PathQuery& PathQuery::operator=(PathQuery&&) = default;

Status PathQuery::New(
    const Path& path,
    const std::vector<QueryItem>& items,
    uint32_t limit,
    uint32_t offset,
    PathQuery& query)
{
    try {
        auto path_buf = wire::Encode(path);
        auto items_buf = wire::Encode(items);
        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
        rust::Slice<const uint8_t> items_slice{items_buf.data(), items_buf.size()};

        auto boxed = grovedb_cxx::grovedb_path_query_new(
            path_slice, items_slice, limit, offset);
        query.m_impl = std::make_unique<Impl>(std::move(boxed));
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::InvalidArgument(e.what());
    }
}

Status PathQuery::NewWithSubquery(
    const Path& path,
    const std::vector<QueryItem>& items,
    uint32_t limit,
    uint32_t offset,
    const Path& subquery_path,
    const std::vector<QueryItem>& subquery_items,
    PathQuery& query)
{
    try {
        auto path_buf = wire::Encode(path);
        auto items_buf = wire::Encode(items);
        auto sq_path_buf = wire::Encode(subquery_path);
        auto sq_items_buf = wire::Encode(subquery_items);

        rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
        rust::Slice<const uint8_t> items_slice{items_buf.data(), items_buf.size()};
        rust::Slice<const uint8_t> sq_path_slice{sq_path_buf.data(), sq_path_buf.size()};
        rust::Slice<const uint8_t> sq_items_slice{sq_items_buf.data(), sq_items_buf.size()};

        auto boxed = grovedb_cxx::grovedb_path_query_new_with_subquery(
            path_slice, items_slice, limit, offset, sq_path_slice, sq_items_slice);
        query.m_impl = std::make_unique<Impl>(std::move(boxed));
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::InvalidArgument(e.what());
    }
}

// ---------------------------------------------------------------------------
// Db::QueryValues
// ---------------------------------------------------------------------------

Status Db::QueryValues(const PathQuery& query, std::vector<Bytes>& values, uint16_t& skipped, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
    Assert(query.m_impl, "called with uninitialized query");
    try {
        auto result = grovedb_cxx::grovedb_query_item_value(
            *m_impl->m_db, *query.m_impl->m_query);
        if (auto s = wire::Decode(std::span<const uint8_t>{result.values.data(), result.values.size()}, values); !s.ok()) {
            return s;
        }
        skipped = result.skipped;
        cost = convert_cost(result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::QueryValues(const PathQuery& query, const Transaction& txn, std::vector<Bytes>& values, uint16_t& skipped, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
    Assert(query.m_impl, "called with uninitialized query");
    Assert(txn.m_impl, "called with uninitialized transaction");
    try {
        auto result = grovedb_cxx::grovedb_query_item_value_with_tx(
            *m_impl->m_db, *query.m_impl->m_query, *txn.m_impl->m_tx);
        if (auto s = wire::Decode(std::span<const uint8_t>{result.values.data(), result.values.size()}, values); !s.ok()) {
            return s;
        }
        skipped = result.skipped;
        cost = convert_cost(result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

// ---------------------------------------------------------------------------
// Db::QueryItemsOrSums
// ---------------------------------------------------------------------------

Status Db::DecodeItemsOrSums(std::span<const uint8_t> data, std::vector<QueryItemOrSum>& results)
{
    wire::Reader r{data};
    uint32_t count{0};
    if (auto s = r.U32(count); !s.ok()) return s;

    results.clear();
    results.reserve(count);
    for (uint32_t i{0}; i < count; ++i) {
        QueryItemOrSum entry;
        uint8_t tag{0};
        if (auto s = r.U8(tag); !s.ok()) return s;

        switch (tag) {
        case 0: { // ItemData
            entry.m_kind = QueryItemOrSum::Kind::ItemData;
            if (auto s = r.Bytes(entry.m_item_data); !s.ok()) return s;
            break;
        }
        case 1: { // SumValue
            entry.m_kind = QueryItemOrSum::Kind::SumValue;
            uint64_t raw{0};
            if (auto s = r.U64(raw); !s.ok()) return s;
            entry.m_sum_value = static_cast<int64_t>(raw);
            break;
        }
        case 2: { // BigSumValue (i128 as 16 LE bytes)
            entry.m_kind = QueryItemOrSum::Kind::BigSumValue;
            uint64_t lo{0}, hi{0};
            if (auto s = r.U64(lo); !s.ok()) return s;
            if (auto s = r.U64(hi); !s.ok()) return s;
            entry.m_big_sum_lo = static_cast<int64_t>(lo);
            entry.m_big_sum_hi = static_cast<int64_t>(hi);
            break;
        }
        case 3: { // CountValue
            entry.m_kind = QueryItemOrSum::Kind::CountValue;
            if (auto s = r.U64(entry.m_count_value); !s.ok()) return s;
            break;
        }
        case 4: { // CountSumValue
            entry.m_kind = QueryItemOrSum::Kind::CountSumValue;
            if (auto s = r.U64(entry.m_count_value); !s.ok()) return s;
            uint64_t raw{0};
            if (auto s = r.U64(raw); !s.ok()) return s;
            entry.m_sum_value = static_cast<int64_t>(raw);
            break;
        }
        case 5: { // ItemDataWithSum
            entry.m_kind = QueryItemOrSum::Kind::ItemDataWithSum;
            if (auto s = r.Bytes(entry.m_item_data); !s.ok()) return s;
            uint64_t raw{0};
            if (auto s = r.U64(raw); !s.ok()) return s;
            entry.m_sum_value = static_cast<int64_t>(raw);
            break;
        }
        default:
            return Status::Corruption("wire: unknown QueryItemOrSum tag");
        }

        results.push_back(std::move(entry));
    }
    return Status::Ok();
}

Status Db::QueryItemsOrSums(const PathQuery& query, std::vector<QueryItemOrSum>& results, uint16_t& skipped, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
    Assert(query.m_impl, "called with uninitialized query");
    try {
        auto result = grovedb_cxx::grovedb_query_item_value_or_sum(
            *m_impl->m_db, *query.m_impl->m_query);
        if (auto s = DecodeItemsOrSums({result.values.data(), result.values.size()}, results); !s.ok()) {
            return s;
        }
        skipped = result.skipped;
        cost = convert_cost(result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::QueryItemsOrSums(const PathQuery& query, const Transaction& txn, std::vector<QueryItemOrSum>& results, uint16_t& skipped, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
    Assert(query.m_impl, "called with uninitialized query");
    Assert(txn.m_impl, "called with uninitialized transaction");
    try {
        auto result = grovedb_cxx::grovedb_query_item_value_or_sum_with_tx(
            *m_impl->m_db, *query.m_impl->m_query, *txn.m_impl->m_tx);
        if (auto s = DecodeItemsOrSums({result.values.data(), result.values.size()}, results); !s.ok()) {
            return s;
        }
        skipped = result.skipped;
        cost = convert_cost(result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

// ---------------------------------------------------------------------------
// Db::QuerySums
// ---------------------------------------------------------------------------

Status Db::DecodeSums(std::span<const uint8_t> data, std::vector<int64_t>& sums)
{
    wire::Reader r{data};
    uint32_t count{0};
    if (auto s = r.U32(count); !s.ok()) return s;

    sums.clear();
    sums.reserve(count);
    for (uint32_t i{0}; i < count; ++i) {
        uint64_t raw{0};
        if (auto s = r.U64(raw); !s.ok()) return s;
        sums.push_back(static_cast<int64_t>(raw));
    }
    return Status::Ok();
}

Status Db::QuerySums(const PathQuery& query, std::vector<int64_t>& sums, uint16_t& skipped, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
    Assert(query.m_impl, "called with uninitialized query");
    try {
        auto result = grovedb_cxx::grovedb_query_sums(
            *m_impl->m_db, *query.m_impl->m_query);
        if (auto s = DecodeSums({result.values.data(), result.values.size()}, sums); !s.ok()) {
            return s;
        }
        skipped = result.skipped;
        cost = convert_cost(result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::QuerySums(const PathQuery& query, const Transaction& txn, std::vector<int64_t>& sums, uint16_t& skipped, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
    Assert(query.m_impl, "called with uninitialized query");
    Assert(txn.m_impl, "called with uninitialized transaction");
    try {
        auto result = grovedb_cxx::grovedb_query_sums_with_tx(
            *m_impl->m_db, *query.m_impl->m_query, *txn.m_impl->m_tx);
        if (auto s = DecodeSums({result.values.data(), result.values.size()}, sums); !s.ok()) {
            return s;
        }
        skipped = result.skipped;
        cost = convert_cost(result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

// ---------------------------------------------------------------------------
// Db::QueryRaw / Db::QueryManyRaw
// ---------------------------------------------------------------------------

Status Db::DecodeQueryResultElements(std::span<const uint8_t> data, std::vector<QueryResultElement>& elements)
{
    wire::Reader r{data};
    uint32_t count{0};
    if (auto s = r.U32(count); !s.ok()) return s;

    elements.clear();
    elements.reserve(count);
    for (uint32_t i{0}; i < count; ++i) {
        QueryResultElement entry;
        uint8_t variant{0};
        if (auto s = r.U8(variant); !s.ok()) return s;

        switch (variant) {
        case 0: { // Element
            entry.m_kind = QueryResultElement::Kind::Element;
            Bytes elem_bytes;
            if (auto s = r.Bytes(elem_bytes); !s.ok()) return s;
            entry.m_element.m_data = std::move(elem_bytes);
            break;
        }
        case 1: { // KeyElementPair
            entry.m_kind = QueryResultElement::Kind::KeyElementPair;
            if (auto s = r.Bytes(entry.m_key); !s.ok()) return s;
            Bytes elem_bytes;
            if (auto s = r.Bytes(elem_bytes); !s.ok()) return s;
            entry.m_element.m_data = std::move(elem_bytes);
            break;
        }
        case 2: { // PathKeyElementTrio
            entry.m_kind = QueryResultElement::Kind::PathKeyElementTrio;
            if (auto s = wire::WireRead(r, entry.m_path); !s.ok()) return s;
            if (auto s = r.Bytes(entry.m_key); !s.ok()) return s;
            Bytes elem_bytes;
            if (auto s = r.Bytes(elem_bytes); !s.ok()) return s;
            entry.m_element.m_data = std::move(elem_bytes);
            break;
        }
        default:
            return Status::Corruption("wire: unknown QueryResultElement variant");
        }

        elements.push_back(std::move(entry));
    }
    return Status::Ok();
}

Status Db::QueryRaw(const PathQuery& query, uint8_t result_type, std::vector<QueryResultElement>& elements, uint16_t& skipped, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
    Assert(query.m_impl, "called with uninitialized query");
    try {
        auto result = grovedb_cxx::grovedb_query_raw(
            *m_impl->m_db, *query.m_impl->m_query, result_type);
        if (auto s = DecodeQueryResultElements({result.values.data(), result.values.size()}, elements); !s.ok()) {
            return s;
        }
        skipped = result.skipped;
        cost = convert_cost(result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::QueryRaw(const PathQuery& query, uint8_t result_type, const Transaction& txn, std::vector<QueryResultElement>& elements, uint16_t& skipped, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
    Assert(query.m_impl, "called with uninitialized query");
    Assert(txn.m_impl, "called with uninitialized transaction");
    try {
        auto result = grovedb_cxx::grovedb_query_raw_with_tx(
            *m_impl->m_db, *query.m_impl->m_query, result_type, *txn.m_impl->m_tx);
        if (auto s = DecodeQueryResultElements({result.values.data(), result.values.size()}, elements); !s.ok()) {
            return s;
        }
        skipped = result.skipped;
        cost = convert_cost(result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Bytes Db::EncodeManyQueries(const std::vector<RawQuerySpec>& queries)
{
    Assert(queries.size() <= std::numeric_limits<uint32_t>::max(),
           "query count exceeds wire protocol limit");
    if (queries.size() > std::numeric_limits<uint32_t>::max()) {
        throw std::overflow_error("query count exceeds wire protocol limit");
    }
    wire::Writer w;
    w.U32(static_cast<uint32_t>(queries.size()));
    for (const auto& q : queries) {
        // Path
        wire::WireWrite(w, q.path);
        // Query items
        wire::WireWrite(w, q.items);
        // Limit and offset
        w.U32(q.limit);
        w.U32(q.offset);
    }
    return w.Take();
}

Status Db::QueryManyRaw(const std::vector<RawQuerySpec>& queries, uint8_t result_type, std::vector<QueryResultElement>& elements, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
    try {
        auto encoded = EncodeManyQueries(queries);
        rust::Slice<const uint8_t> encoded_slice{encoded.data(), encoded.size()};
        auto result = grovedb_cxx::grovedb_query_many_raw(
            *m_impl->m_db, encoded_slice, result_type);
        if (auto s = DecodeQueryResultElements({result.values.data(), result.values.size()}, elements); !s.ok()) {
            return s;
        }
        cost = convert_cost(result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

// ---------------------------------------------------------------------------
// Db::QueryKeysOptional / Db::QueryRawKeysOptional
// ---------------------------------------------------------------------------

Status Db::DecodePathKeyElements(std::span<const uint8_t> data, std::vector<PathKeyElement>& results)
{
    wire::Reader r{data};
    uint32_t count{0};
    if (auto s = r.U32(count); !s.ok()) return s;

    results.clear();
    results.reserve(count);
    for (uint32_t i{0}; i < count; ++i) {
        PathKeyElement entry;
        if (auto s = wire::WireRead(r, entry.m_path); !s.ok()) return s;
        if (auto s = r.Bytes(entry.m_key); !s.ok()) return s;

        uint8_t has_elem{0};
        if (auto s = r.U8(has_elem); !s.ok()) return s;
        if (has_elem) {
            Bytes elem_bytes;
            if (auto s = r.Bytes(elem_bytes); !s.ok()) return s;
            Element elem;
            elem.m_data = std::move(elem_bytes);
            entry.m_element = std::move(elem);
        }

        results.push_back(std::move(entry));
    }
    return Status::Ok();
}

Status Db::QueryKeysOptional(const PathQuery& query, std::vector<PathKeyElement>& results, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
    Assert(query.m_impl, "called with uninitialized query");
    try {
        auto result = grovedb_cxx::grovedb_query_keys_optional(
            *m_impl->m_db, *query.m_impl->m_query);
        if (auto s = DecodePathKeyElements({result.values.data(), result.values.size()}, results); !s.ok()) {
            return s;
        }
        cost = convert_cost(result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::QueryKeysOptional(const PathQuery& query, const Transaction& txn, std::vector<PathKeyElement>& results, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
    Assert(query.m_impl, "called with uninitialized query");
    Assert(txn.m_impl, "called with uninitialized transaction");
    try {
        auto result = grovedb_cxx::grovedb_query_keys_optional_with_tx(
            *m_impl->m_db, *query.m_impl->m_query, *txn.m_impl->m_tx);
        if (auto s = DecodePathKeyElements({result.values.data(), result.values.size()}, results); !s.ok()) {
            return s;
        }
        cost = convert_cost(result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::QueryRawKeysOptional(const PathQuery& query, std::vector<PathKeyElement>& results, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
    Assert(query.m_impl, "called with uninitialized query");
    try {
        auto result = grovedb_cxx::grovedb_query_raw_keys_optional(
            *m_impl->m_db, *query.m_impl->m_query);
        if (auto s = DecodePathKeyElements({result.values.data(), result.values.size()}, results); !s.ok()) {
            return s;
        }
        cost = convert_cost(result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::QueryRawKeysOptional(const PathQuery& query, const Transaction& txn, std::vector<PathKeyElement>& results, OperationCost& cost)
{
    Assert(m_impl, "called on uninitialized database");
    Assert(query.m_impl, "called with uninitialized query");
    Assert(txn.m_impl, "called with uninitialized transaction");
    try {
        auto result = grovedb_cxx::grovedb_query_raw_keys_optional_with_tx(
            *m_impl->m_db, *query.m_impl->m_query, *txn.m_impl->m_tx);
        if (auto s = DecodePathKeyElements({result.values.data(), result.values.size()}, results); !s.ok()) {
            return s;
        }
        cost = convert_cost(result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

} // namespace grovedb

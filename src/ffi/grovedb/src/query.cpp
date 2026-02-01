// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifdef HAVE_GROVEDB_CONFIG
#include <config/grovedb-config.h>
#endif

#include <grovedb/query.h>

#include <rust/grovedb_cxx/lib.h>
#include <types/query.h>

#include <string>

namespace {
/** Encode a path into the flat wire format: [u32 count][u32 len][bytes]... */
std::vector<uint8_t> encode_path(const grovedb::Path& path)
{
    size_t total{4};
    for (const auto& seg : path) total += 4 + seg.size();

    std::vector<uint8_t> buf;
    buf.reserve(total);

    auto push_u32 = [&buf](uint32_t v) {
        buf.push_back(static_cast<uint8_t>(v));
        buf.push_back(static_cast<uint8_t>(v >> 8));
        buf.push_back(static_cast<uint8_t>(v >> 16));
        buf.push_back(static_cast<uint8_t>(v >> 24));
    };

    push_u32(static_cast<uint32_t>(path.size()));
    for (const auto& seg : path) {
        push_u32(static_cast<uint32_t>(seg.size()));
        buf.insert(buf.end(), seg.begin(), seg.end());
    }

    return buf;
}

/** Encode query items into the wire format expected by the CXX bridge.
 *
 *  Layout: [u32 item_count] then per item [u8 kind][kind-specific data]
 *  where data fields are length-prefixed bytes: [u32 len][bytes].
 */
std::vector<uint8_t> encode_query_items(const std::vector<grovedb::QueryItem>& items)
{
    // Estimate size: 4 (count) + per item ~(1 + 4 + avg_size)
    std::vector<uint8_t> buf;
    buf.reserve(4 + items.size() * 16);

    auto push_u32 = [&buf](uint32_t v) {
        buf.push_back(static_cast<uint8_t>(v));
        buf.push_back(static_cast<uint8_t>(v >> 8));
        buf.push_back(static_cast<uint8_t>(v >> 16));
        buf.push_back(static_cast<uint8_t>(v >> 24));
    };

    auto push_bytes = [&](const grovedb::Bytes& b) {
        push_u32(static_cast<uint32_t>(b.size()));
        buf.insert(buf.end(), b.begin(), b.end());
    };

    push_u32(static_cast<uint32_t>(items.size()));
    for (const auto& item : items) {
        buf.push_back(item.kind());
        switch (item.kind()) {
        case 0: // Key
            push_bytes(item.first());
            break;
        case 1: // Range
        case 2: // RangeInclusive
        case 8: // RangeAfterTo
        case 9: // RangeAfterToInclusive
            push_bytes(item.first());
            push_bytes(item.second());
            break;
        case 3: // RangeFull
            break;
        case 4: // RangeFrom
        case 5: // RangeTo
        case 6: // RangeToInclusive
        case 7: // RangeAfter
            push_bytes(item.first());
            break;
        }
    }

    return buf;
}
} // anonymous namespace

namespace grovedb {

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
        auto path_buf = encode_path(path);
        auto items_buf = encode_query_items(items);
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
        auto path_buf = encode_path(path);
        auto items_buf = encode_query_items(items);
        auto sq_path_buf = encode_path(subquery_path);
        auto sq_items_buf = encode_query_items(subquery_items);

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

} // namespace grovedb

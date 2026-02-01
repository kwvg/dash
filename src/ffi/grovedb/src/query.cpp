// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifdef HAVE_GROVEDB_CONFIG
#include <config/grovedb-config.h>
#endif

#include <grovedb/query.h>
#include <grovedb/wire.h>

#include <rust/grovedb_cxx/lib.h>
#include <types/query.h>

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

} // namespace grovedb

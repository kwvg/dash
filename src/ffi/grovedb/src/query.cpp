// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <decode_internal.h>
#include <types/db.h>
#include <types/query.h>
#include <types/transaction.h>
#include <util/ffi.h>

#include <grovedb/query.h>
#include <grovedb/wire.h>

#include <rust/grovedb_cxx/lib.h>

#include <cstdint>
#include <limits>
#include <span>
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
    break;
  }
}

Result<QueryItem, wire::Error> QueryItem::Decode(wire::Reader& r)
{
  auto kind_r = r.U8();
  if (!kind_r) {
    return Err(kind_r.error());
  }
  uint8_t kind = *kind_r;

  switch (kind) {
  case 0: // Key
    return r.Bytes().transform([](Bytes a) { return QueryItem::Key(a); });
  case 1: // Range
    return r.Bytes().and_then([&r](Bytes a) {
      return r.Bytes().transform([&a](Bytes b) { return QueryItem::Range(a, b); });
    });
  case 2: // RangeInclusive
    return r.Bytes().and_then([&r](Bytes a) {
      return r.Bytes().transform([&a](Bytes b) { return QueryItem::RangeInclusive(a, b); });
    });
  case 3: // RangeFull
    return QueryItem::RangeFull();
  case 4: // RangeFrom
    return r.Bytes().transform([](Bytes a) { return QueryItem::RangeFrom(a); });
  case 5: // RangeTo
    return r.Bytes().transform([](Bytes a) { return QueryItem::RangeTo(a); });
  case 6: // RangeToInclusive
    return r.Bytes().transform([](Bytes a) { return QueryItem::RangeToInclusive(a); });
  case 7: // RangeAfter
    return r.Bytes().transform([](Bytes a) { return QueryItem::RangeAfter(a); });
  case 8: // RangeAfterTo
    return r.Bytes().and_then([&r](Bytes a) {
      return r.Bytes().transform([&a](Bytes b) { return QueryItem::RangeAfterTo(a, b); });
    });
  case 9: // RangeAfterToInclusive
    return r.Bytes().and_then([&r](Bytes a) {
      return r.Bytes().transform([&a](Bytes b) { return QueryItem::RangeAfterToInclusive(a, b); });
    });
  default:
    return Err(wire::Error::Corruption);
  }
}

// ---------------------------------------------------------------------------
// QueryItem factories
// ---------------------------------------------------------------------------

QueryItem QueryItem::Key(const Bytes& key)
{
  return {0, key, {}};
}
QueryItem QueryItem::Range(const Bytes& start, const Bytes& end)
{
  return {1, start, end};
}
QueryItem QueryItem::RangeInclusive(const Bytes& start, const Bytes& end)
{
  return {2, start, end};
}
QueryItem QueryItem::RangeFull()
{
  return {3, {}, {}};
}
QueryItem QueryItem::RangeFrom(const Bytes& start)
{
  return {4, start, {}};
}
QueryItem QueryItem::RangeTo(const Bytes& end)
{
  return {5, end, {}};
}
QueryItem QueryItem::RangeToInclusive(const Bytes& end)
{
  return {6, end, {}};
}
QueryItem QueryItem::RangeAfter(const Bytes& after)
{
  return {7, after, {}};
}
QueryItem QueryItem::RangeAfterTo(const Bytes& after, const Bytes& to)
{
  return {8, after, to};
}
QueryItem QueryItem::RangeAfterToInclusive(const Bytes& after, const Bytes& to)
{
  return {9, after, to};
}

// ---------------------------------------------------------------------------
// PathQuery
// ---------------------------------------------------------------------------

PathQuery::PathQuery() = default;
PathQuery::~PathQuery() = default;
PathQuery::PathQuery(PathQuery&&) = default;
PathQuery& PathQuery::operator=(PathQuery&&) = default;

Result<PathQuery, Error> PathQuery::New(
    const Path& path, const std::vector<QueryItem>& items, uint32_t limit, uint32_t offset
)
{
  return CallFFI([&]() -> Result<PathQuery, Error> {
    auto path_buf = wire::Encode(path);
    auto items_buf = wire::Encode(items);

    auto boxed =
        grovedb_cxx::grovedb_path_query_new(ToSlice(path_buf), ToSlice(items_buf), limit, offset);
    PathQuery query;
    query.m_impl = std::make_unique<Impl>(std::move(boxed));
    return query;
  });
}

Result<PathQuery, Error> PathQuery::NewWithSubquery(
    const Path& path,
    const std::vector<QueryItem>& items,
    uint32_t limit,
    uint32_t offset,
    const Path& subquery_path,
    const std::vector<QueryItem>& subquery_items
)
{
  return CallFFI([&]() -> Result<PathQuery, Error> {
    auto path_buf = wire::Encode(path);
    auto items_buf = wire::Encode(items);
    auto sq_path_buf = wire::Encode(subquery_path);
    auto sq_items_buf = wire::Encode(subquery_items);

    auto boxed = grovedb_cxx::grovedb_path_query_new_with_subquery(
        ToSlice(path_buf),
        ToSlice(items_buf),
        limit,
        offset,
        ToSlice(sq_path_buf),
        ToSlice(sq_items_buf)
    );
    PathQuery query;
    query.m_impl = std::make_unique<Impl>(std::move(boxed));
    return query;
  });
}

// ---------------------------------------------------------------------------
// Wire decoding helpers (private to this TU)
// ---------------------------------------------------------------------------

namespace {
Result<std::vector<Bytes>, wire::Error> DecodeValues(std::span<const uint8_t> data)
{
  wire::Reader r{data};
  return wire::Read<std::vector<Bytes>>(r);
}

Result<std::vector<QueryItemOrSum>, wire::Error> DecodeItemsOrSums(std::span<const uint8_t> data)
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

  std::vector<QueryItemOrSum> results;
  results.reserve(count);
  for (uint32_t i{0}; i < count; ++i) {
    QueryItemOrSum entry;
    auto tag_r = r.U8();
    if (!tag_r) {
      return Err(tag_r.error());
    }

    switch (*tag_r) {
    case 0: { // ItemData
      entry.m_kind = QueryItemOrSumKind::ItemData;
      auto bytes_r = r.Bytes();
      if (!bytes_r) {
        return Err(bytes_r.error());
      }
      entry.m_item_data = std::move(*bytes_r);
      break;
    }
    case 1: { // SumValue
      entry.m_kind = QueryItemOrSumKind::SumValue;
      auto raw_r = r.U64();
      if (!raw_r) {
        return Err(raw_r.error());
      }
      entry.m_sum_value = static_cast<int64_t>(*raw_r);
      break;
    }
    case 2: { // BigSumValue (i128 as 16 LE bytes)
      entry.m_kind = QueryItemOrSumKind::BigSumValue;
      auto lo_r = r.U64();
      if (!lo_r) {
        return Err(lo_r.error());
      }
      auto hi_r = r.U64();
      if (!hi_r) {
        return Err(hi_r.error());
      }
      entry.m_big_sum_lo = static_cast<int64_t>(*lo_r);
      entry.m_big_sum_hi = static_cast<int64_t>(*hi_r);
      break;
    }
    case 3: { // CountValue
      entry.m_kind = QueryItemOrSumKind::CountValue;
      auto count_val_r = r.U64();
      if (!count_val_r) {
        return Err(count_val_r.error());
      }
      entry.m_count_value = *count_val_r;
      break;
    }
    case 4: { // CountSumValue
      entry.m_kind = QueryItemOrSumKind::CountSumValue;
      auto cv_r = r.U64();
      if (!cv_r) {
        return Err(cv_r.error());
      }
      entry.m_count_value = *cv_r;
      auto sv_r = r.U64();
      if (!sv_r) {
        return Err(sv_r.error());
      }
      entry.m_sum_value = static_cast<int64_t>(*sv_r);
      break;
    }
    case 5: { // ItemDataWithSum
      entry.m_kind = QueryItemOrSumKind::ItemDataWithSum;
      auto bytes_r = r.Bytes();
      if (!bytes_r) {
        return Err(bytes_r.error());
      }
      entry.m_item_data = std::move(*bytes_r);
      auto sv_r = r.U64();
      if (!sv_r) {
        return Err(sv_r.error());
      }
      entry.m_sum_value = static_cast<int64_t>(*sv_r);
      break;
    }
    default:
      return Err(wire::Error::Corruption);
    }

    results.push_back(std::move(entry));
  }
  return results;
}

Result<std::vector<int64_t>, wire::Error> DecodeSums(std::span<const uint8_t> data)
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

  std::vector<int64_t> sums;
  sums.reserve(count);
  for (uint32_t i{0}; i < count; ++i) {
    auto raw_r = r.U64();
    if (!raw_r) {
      return Err(raw_r.error());
    }
    sums.push_back(static_cast<int64_t>(*raw_r));
  }
  return sums;
}

Result<std::vector<QueryResultElement>, wire::Error>
DecodeQueryResultElements(std::span<const uint8_t> data)
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

  std::vector<QueryResultElement> elements;
  elements.reserve(count);
  for (uint32_t i{0}; i < count; ++i) {
    QueryResultElement entry;
    auto variant_r = r.U8();
    if (!variant_r) {
      return Err(variant_r.error());
    }

    switch (*variant_r) {
    case 0: { // Element
      entry.m_kind = QueryResultKind::Element;
      auto elem_r = r.Bytes();
      if (!elem_r) {
        return Err(elem_r.error());
      }
      entry.m_element = Element::FromData(std::move(*elem_r));
      break;
    }
    case 1: { // KeyElementPair
      entry.m_kind = QueryResultKind::KeyElementPair;
      auto key_r = r.Bytes();
      if (!key_r) {
        return Err(key_r.error());
      }
      entry.m_key = std::move(*key_r);
      auto elem_r = r.Bytes();
      if (!elem_r) {
        return Err(elem_r.error());
      }
      entry.m_element = Element::FromData(std::move(*elem_r));
      break;
    }
    case 2: { // PathKeyElementTrio
      entry.m_kind = QueryResultKind::PathKeyElementTrio;
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
      auto elem_r = r.Bytes();
      if (!elem_r) {
        return Err(elem_r.error());
      }
      entry.m_element = Element::FromData(std::move(*elem_r));
      break;
    }
    default:
      return Err(wire::Error::Corruption);
    }

    elements.push_back(std::move(entry));
  }
  return elements;
}

/// Encode a list of RawQuerySpec into the wire format for QueryManyRaw.
Bytes EncodeManyQueries(const std::vector<Db::RawQuerySpec>& queries)
{
  wire::Writer w;
  w.U32(static_cast<uint32_t>(queries.size()));
  for (const auto& q : queries) {
    wire::Write(w, q.path);
    wire::Write(w, q.items);
    w.U32(q.limit);
    w.U32(q.offset);
  }
  return w.Take();
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Db::QueryValues
// ---------------------------------------------------------------------------

Result<Costed<QueryData<std::vector<Bytes>>>, Error> Db::QueryValues(const PathQuery& query)
{
  return CallFFI([&]() -> Result<Costed<QueryData<std::vector<Bytes>>>, Error> {
    auto result = grovedb_cxx::grovedb_query_item_value(*m_impl->m_db, *query.m_impl->m_query);
    return DecodeValues({result.values.data(), result.values.size()})
        .transform_error([](wire::Error) {
          return Error::Corruption("failed to decode query values");
        })
        .transform([&](std::vector<Bytes> values) {
          return Costed<QueryData<std::vector<Bytes>>>{
              {std::move(values), result.skipped}, convert_cost(result.cost)
          };
        });
  });
}

Result<Costed<QueryData<std::vector<Bytes>>>, Error>
Db::QueryValues(const PathQuery& query, const Transaction& txn)
{
  return CallFFI([&]() -> Result<Costed<QueryData<std::vector<Bytes>>>, Error> {
    auto result = grovedb_cxx::grovedb_query_item_value_with_tx(
        *m_impl->m_db, *query.m_impl->m_query, *txn.m_impl->m_tx
    );
    return DecodeValues({result.values.data(), result.values.size()})
        .transform_error([](wire::Error) {
          return Error::Corruption("failed to decode query values");
        })
        .transform([&](std::vector<Bytes> values) {
          return Costed<QueryData<std::vector<Bytes>>>{
              {std::move(values), result.skipped}, convert_cost(result.cost)
          };
        });
  });
}

// ---------------------------------------------------------------------------
// Db::QueryItemsOrSums
// ---------------------------------------------------------------------------

Result<Costed<QueryData<std::vector<QueryItemOrSum>>>, Error>
Db::QueryItemsOrSums(const PathQuery& query)
{
  return CallFFI([&]() -> Result<Costed<QueryData<std::vector<QueryItemOrSum>>>, Error> {
    auto result =
        grovedb_cxx::grovedb_query_item_value_or_sum(*m_impl->m_db, *query.m_impl->m_query);
    return DecodeItemsOrSums({result.values.data(), result.values.size()})
        .transform_error([](wire::Error) {
          return Error::Corruption("failed to decode items-or-sums");
        })
        .transform([&](std::vector<QueryItemOrSum> items) {
          return Costed<QueryData<std::vector<QueryItemOrSum>>>{
              {std::move(items), result.skipped}, convert_cost(result.cost)
          };
        });
  });
}

Result<Costed<QueryData<std::vector<QueryItemOrSum>>>, Error>
Db::QueryItemsOrSums(const PathQuery& query, const Transaction& txn)
{
  return CallFFI([&]() -> Result<Costed<QueryData<std::vector<QueryItemOrSum>>>, Error> {
    auto result = grovedb_cxx::grovedb_query_item_value_or_sum_with_tx(
        *m_impl->m_db, *query.m_impl->m_query, *txn.m_impl->m_tx
    );
    return DecodeItemsOrSums({result.values.data(), result.values.size()})
        .transform_error([](wire::Error) {
          return Error::Corruption("failed to decode items-or-sums");
        })
        .transform([&](std::vector<QueryItemOrSum> items) {
          return Costed<QueryData<std::vector<QueryItemOrSum>>>{
              {std::move(items), result.skipped}, convert_cost(result.cost)
          };
        });
  });
}

// ---------------------------------------------------------------------------
// Db::QuerySums
// ---------------------------------------------------------------------------

Result<Costed<QueryData<std::vector<int64_t>>>, Error> Db::QuerySums(const PathQuery& query)
{
  return CallFFI([&]() -> Result<Costed<QueryData<std::vector<int64_t>>>, Error> {
    auto result = grovedb_cxx::grovedb_query_sums(*m_impl->m_db, *query.m_impl->m_query);
    return DecodeSums({result.values.data(), result.values.size()})
        .transform_error([](wire::Error) { return Error::Corruption("failed to decode sums"); })
        .transform([&](std::vector<int64_t> sums) {
          return Costed<QueryData<std::vector<int64_t>>>{
              {std::move(sums), result.skipped}, convert_cost(result.cost)
          };
        });
  });
}

Result<Costed<QueryData<std::vector<int64_t>>>, Error>
Db::QuerySums(const PathQuery& query, const Transaction& txn)
{
  return CallFFI([&]() -> Result<Costed<QueryData<std::vector<int64_t>>>, Error> {
    auto result = grovedb_cxx::grovedb_query_sums_with_tx(
        *m_impl->m_db, *query.m_impl->m_query, *txn.m_impl->m_tx
    );
    return DecodeSums({result.values.data(), result.values.size()})
        .transform_error([](wire::Error) { return Error::Corruption("failed to decode sums"); })
        .transform([&](std::vector<int64_t> sums) {
          return Costed<QueryData<std::vector<int64_t>>>{
              {std::move(sums), result.skipped}, convert_cost(result.cost)
          };
        });
  });
}

// ---------------------------------------------------------------------------
// Db::QueryRaw
// ---------------------------------------------------------------------------

Result<Costed<QueryData<std::vector<QueryResultElement>>>, Error>
Db::QueryRaw(const PathQuery& query, uint8_t result_type)
{
  return CallFFI([&]() -> Result<Costed<QueryData<std::vector<QueryResultElement>>>, Error> {
    auto result =
        grovedb_cxx::grovedb_query_raw(*m_impl->m_db, *query.m_impl->m_query, result_type);
    return DecodeQueryResultElements({result.values.data(), result.values.size()})
        .transform_error([](wire::Error) {
          return Error::Corruption("failed to decode raw query results");
        })
        .transform([&](std::vector<QueryResultElement> elems) {
          return Costed<QueryData<std::vector<QueryResultElement>>>{
              {std::move(elems), result.skipped}, convert_cost(result.cost)
          };
        });
  });
}

Result<Costed<QueryData<std::vector<QueryResultElement>>>, Error>
Db::QueryRaw(const PathQuery& query, uint8_t result_type, const Transaction& txn)
{
  return CallFFI([&]() -> Result<Costed<QueryData<std::vector<QueryResultElement>>>, Error> {
    auto result = grovedb_cxx::grovedb_query_raw_with_tx(
        *m_impl->m_db, *query.m_impl->m_query, result_type, *txn.m_impl->m_tx
    );
    return DecodeQueryResultElements({result.values.data(), result.values.size()})
        .transform_error([](wire::Error) {
          return Error::Corruption("failed to decode raw query results");
        })
        .transform([&](std::vector<QueryResultElement> elems) {
          return Costed<QueryData<std::vector<QueryResultElement>>>{
              {std::move(elems), result.skipped}, convert_cost(result.cost)
          };
        });
  });
}

// ---------------------------------------------------------------------------
// Db::QueryManyRaw
// ---------------------------------------------------------------------------

Result<Costed<std::vector<QueryResultElement>>, Error>
Db::QueryManyRaw(const std::vector<RawQuerySpec>& queries, uint8_t result_type)
{
  return CallFFI([&]() -> Result<Costed<std::vector<QueryResultElement>>, Error> {
    auto encoded = EncodeManyQueries(queries);
    auto result = grovedb_cxx::grovedb_query_many_raw(*m_impl->m_db, ToSlice(encoded), result_type);
    return DecodeQueryResultElements({result.values.data(), result.values.size()})
        .transform_error([](wire::Error) {
          return Error::Corruption("failed to decode many-raw query results");
        })
        .transform([&](std::vector<QueryResultElement> elems) {
          return Costed<std::vector<QueryResultElement>>{
              std::move(elems), convert_cost(result.cost)
          };
        });
  });
}

// ---------------------------------------------------------------------------
// Db::QueryKeysOptional / Db::QueryRawKeysOptional
// ---------------------------------------------------------------------------

Result<Costed<std::vector<PathKeyElement>>, Error> Db::QueryKeysOptional(const PathQuery& query)
{
  return CallFFI([&]() -> Result<Costed<std::vector<PathKeyElement>>, Error> {
    auto result = grovedb_cxx::grovedb_query_keys_optional(*m_impl->m_db, *query.m_impl->m_query);
    return DecodePathKeyElements({result.values.data(), result.values.size()})
        .transform_error([](wire::Error) {
          return Error::Corruption("failed to decode keys-optional results");
        })
        .transform([&](std::vector<PathKeyElement> entries) {
          return Costed<std::vector<PathKeyElement>>{std::move(entries), convert_cost(result.cost)};
        });
  });
}

Result<Costed<std::vector<PathKeyElement>>, Error>
Db::QueryKeysOptional(const PathQuery& query, const Transaction& txn)
{
  return CallFFI([&]() -> Result<Costed<std::vector<PathKeyElement>>, Error> {
    auto result = grovedb_cxx::grovedb_query_keys_optional_with_tx(
        *m_impl->m_db, *query.m_impl->m_query, *txn.m_impl->m_tx
    );
    return DecodePathKeyElements({result.values.data(), result.values.size()})
        .transform_error([](wire::Error) {
          return Error::Corruption("failed to decode keys-optional results");
        })
        .transform([&](std::vector<PathKeyElement> entries) {
          return Costed<std::vector<PathKeyElement>>{std::move(entries), convert_cost(result.cost)};
        });
  });
}

Result<Costed<std::vector<PathKeyElement>>, Error> Db::QueryRawKeysOptional(const PathQuery& query)
{
  return CallFFI([&]() -> Result<Costed<std::vector<PathKeyElement>>, Error> {
    auto result =
        grovedb_cxx::grovedb_query_raw_keys_optional(*m_impl->m_db, *query.m_impl->m_query);
    return DecodePathKeyElements({result.values.data(), result.values.size()})
        .transform_error([](wire::Error) {
          return Error::Corruption("failed to decode raw-keys-optional results");
        })
        .transform([&](std::vector<PathKeyElement> entries) {
          return Costed<std::vector<PathKeyElement>>{std::move(entries), convert_cost(result.cost)};
        });
  });
}

Result<Costed<std::vector<PathKeyElement>>, Error>
Db::QueryRawKeysOptional(const PathQuery& query, const Transaction& txn)
{
  return CallFFI([&]() -> Result<Costed<std::vector<PathKeyElement>>, Error> {
    auto result = grovedb_cxx::grovedb_query_raw_keys_optional_with_tx(
        *m_impl->m_db, *query.m_impl->m_query, *txn.m_impl->m_tx
    );
    return DecodePathKeyElements({result.values.data(), result.values.size()})
        .transform_error([](wire::Error) {
          return Error::Corruption("failed to decode raw-keys-optional results");
        })
        .transform([&](std::vector<PathKeyElement> entries) {
          return Costed<std::vector<PathKeyElement>>{std::move(entries), convert_cost(result.cost)};
        });
  });
}
} // namespace grovedb

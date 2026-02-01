// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GROVEDB_WIRE_H
#define GROVEDB_WIRE_H

#include <grovedb/query.h>
#include <grovedb/status.h>
#include <grovedb/types.h>

#include <util/wire.h>

#include <concepts>
#include <cstdint>
#include <span>
#include <vector>

namespace grovedb {
namespace wire {

// ---------------------------------------------------------------------------
// Concepts
// ---------------------------------------------------------------------------

/** A type that can be serialized to a wire::Writer. */
template<typename T>
concept Writable = requires(Writer& w, const T& v) {
    WireWrite(w, v);
};

/** A type that can be deserialized from a wire::Reader. */
template<typename T>
concept Readable = requires(Reader& r, T& v) {
    { WireRead(r, v) } -> std::same_as<Status>;
};

/** A type that supports both serialization and deserialization. */
template<typename T>
concept Serializable = Writable<T> && Readable<T>;

// ---------------------------------------------------------------------------
// ADL free functions: Bytes
// ---------------------------------------------------------------------------

/**
 * Encode a byte vector as length-prefixed bytes.
 *
 * @param[in] w  Writer to append to.
 * @param[in] v  Byte vector to encode.
 */
inline void WireWrite(Writer& w, const Bytes& v)
{
    w.Bytes(v);
}

/**
 * Decode a byte vector from length-prefixed bytes.
 *
 * @param[in]  r  Reader to consume from.
 * @param[out] v  Receives the decoded byte vector.
 * @return Status::Ok() on success; an error Status otherwise.
 */
inline Status WireRead(Reader& r, Bytes& v)
{
    return r.Bytes(v);
}

// ---------------------------------------------------------------------------
// ADL free functions: Path
// ---------------------------------------------------------------------------

/**
 * Encode a path as [u32 segment_count][u32 len + bytes]...
 *
 * @param[in] w     Writer to append to.
 * @param[in] path  Path to encode.
 */
inline void WireWrite(Writer& w, const Path& path)
{
    w.U32(static_cast<uint32_t>(path.size()));
    for (const auto& seg : path) {
        w.Bytes(seg);
    }
}

/**
 * Decode a path from [u32 segment_count][u32 len + bytes]...
 *
 * @param[in]  r     Reader to consume from.
 * @param[out] path  Receives the decoded path.
 * @return Status::Ok() on success; an error Status otherwise.
 */
inline Status WireRead(Reader& r, Path& path)
{
    uint32_t count{0};
    if (auto s = r.U32(count); !s.ok()) return s;
    path.clear();
    path.reserve(count);
    for (uint32_t i{0}; i < count; ++i) {
        Bytes seg;
        if (auto s = r.Bytes(seg); !s.ok()) return s;
        path.push_back(std::move(seg));
    }
    return Status::Ok();
}

// ---------------------------------------------------------------------------
// ADL free functions: QueryItem
// ---------------------------------------------------------------------------

/**
 * Encode a QueryItem as [u8 kind][kind-specific fields].
 *
 * Each field is length-prefixed bytes.  The layout matches the wire format
 * expected by the CXX bridge.
 *
 * @param[in] w     Writer to append to.
 * @param[in] item  QueryItem to encode.
 */
inline void WireWrite(Writer& w, const QueryItem& item)
{
    w.U8(item.kind());
    switch (item.kind()) {
    case 0: // Key
        w.Bytes(item.first());
        break;
    case 1: // Range
    case 2: // RangeInclusive
    case 8: // RangeAfterTo
    case 9: // RangeAfterToInclusive
        w.Bytes(item.first());
        w.Bytes(item.second());
        break;
    case 3: // RangeFull
        break;
    case 4: // RangeFrom
    case 5: // RangeTo
    case 6: // RangeToInclusive
    case 7: // RangeAfter
        w.Bytes(item.first());
        break;
    }
}

/**
 * Decode a QueryItem from [u8 kind][kind-specific fields].
 *
 * Reconstructs QueryItem via factory methods.
 *
 * @param[in]  r     Reader to consume from.
 * @param[out] item  Receives the decoded QueryItem.
 * @return Status::Ok() on success; an error Status otherwise.
 */
inline Status WireRead(Reader& r, QueryItem& item)
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
// Generic vector support
// ---------------------------------------------------------------------------

/**
 * Encode a vector as [u32 count][element]...
 *
 * @param[in] w  Writer to append to.
 * @param[in] v  Vector of writable elements.
 */
template<Writable T>
void WireWrite(Writer& w, const std::vector<T>& v)
{
    w.U32(static_cast<uint32_t>(v.size()));
    for (const auto& elem : v) {
        WireWrite(w, elem);
    }
}

/**
 * Decode a vector from [u32 count][element]...
 *
 * @param[in]  r  Reader to consume from.
 * @param[out] v  Receives the decoded vector.
 * @return Status::Ok() on success; an error Status otherwise.
 */
template<Readable T>
Status WireRead(Reader& r, std::vector<T>& v)
{
    uint32_t count{0};
    if (auto s = r.U32(count); !s.ok()) return s;
    v.clear();
    v.reserve(count);
    for (uint32_t i{0}; i < count; ++i) {
        v.emplace_back();
        if (auto s = WireRead(r, v.back()); !s.ok()) return s;
    }
    return Status::Ok();
}

// ---------------------------------------------------------------------------
// Convenience functions
// ---------------------------------------------------------------------------

/**
 * Encode a value to a byte vector.
 *
 * @param[in] v  The value to encode.
 * @return The encoded bytes.
 */
template<Writable T>
std::vector<uint8_t> Encode(const T& v)
{
    Writer w;
    WireWrite(w, v);
    return w.Take();
}

/**
 * Decode a value from a byte span.
 *
 * @param[in]  data  Raw wire bytes.
 * @param[out] v     Receives the decoded value.
 * @return Status::Ok() on success; an error Status otherwise.
 */
template<Readable T>
Status Decode(std::span<const uint8_t> data, T& v)
{
    Reader r{data};
    return WireRead(r, v);
}

// ---------------------------------------------------------------------------
// Static assertions
// ---------------------------------------------------------------------------

static_assert(Serializable<Bytes>);
static_assert(Serializable<Path>);
static_assert(Serializable<QueryItem>);

} // namespace wire
} // namespace grovedb

#endif // GROVEDB_WIRE_H

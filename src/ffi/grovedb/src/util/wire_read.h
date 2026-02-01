// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GROVEDB_UTIL_WIRE_READ_H
#define GROVEDB_UTIL_WIRE_READ_H

#include <grovedb/status.h>
#include <grovedb/types.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace grovedb {
namespace wire {

/**
 * Sequential reader for little-endian wire format.
 *
 * Reads from a borrowed byte span with an advancing cursor.
 * Every read returns Status; callers must check before using the output.
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
     * @param[out] v  Receives the decoded value.
     * @return Status::Ok() on success; Status::Corruption() on buffer underflow.
     */
    [[nodiscard]] Status U8(uint8_t& v);

    /**
     * Read an unsigned 16-bit integer from little-endian byte order.
     *
     * @param[out] v  Receives the decoded value.
     * @return Status::Ok() on success; Status::Corruption() on buffer underflow.
     */
    [[nodiscard]] Status U16(uint16_t& v);

    /**
     * Read an unsigned 32-bit integer from little-endian byte order.
     *
     * @param[out] v  Receives the decoded value.
     * @return Status::Ok() on success; Status::Corruption() on buffer underflow.
     */
    [[nodiscard]] Status U32(uint32_t& v);

    /**
     * Read an unsigned 64-bit integer from little-endian byte order.
     *
     * @param[out] v  Receives the decoded value.
     * @return Status::Ok() on success; Status::Corruption() on buffer underflow.
     */
    [[nodiscard]] Status U64(uint64_t& v);

    ///@}

    /**
     * Read exactly `out.size()` raw bytes.
     *
     * @param[out] out  Span to fill with the next bytes from the buffer.
     * @return Status::Ok() on success; Status::Corruption() on buffer underflow.
     */
    [[nodiscard]] Status Raw(std::span<uint8_t> out);

    /**
     * Read length-prefixed bytes (u32 length + data).
     *
     * @param[out] out  Receives the decoded byte vector.
     * @return Status::Ok() on success; Status::Corruption() on buffer underflow.
     */
    [[nodiscard]] Status Bytes(grovedb::Bytes& out);

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

} // namespace wire
} // namespace grovedb

#endif // GROVEDB_UTIL_WIRE_READ_H

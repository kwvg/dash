// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GROVEDB_UTIL_WIRE_WRITE_H
#define GROVEDB_UTIL_WIRE_WRITE_H

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace grovedb {
namespace wire {

/**
 * Sequential writer for little-endian wire format.
 *
 * Appends to an internal byte buffer.  Write operations are infallible;
 * allocation failure is catastrophic (throws std::bad_alloc).
 */
class Writer
{
public:
    Writer() = default;

    /**
     * Construct a writer with a size hint for pre-allocation.
     *
     * @param[in] reserve_hint  Number of bytes to pre-allocate.
     */
    explicit Writer(size_t reserve_hint);

    /** @name Primitive writes (native to LE) */
    ///@{

    /**
     * Write an unsigned 8-bit integer.
     *
     * @param[in] v  Value to write.
     */
    void U8(uint8_t v);

    /**
     * Write an unsigned 16-bit integer in little-endian byte order.
     *
     * @param[in] v  Value to write.
     */
    void U16(uint16_t v);

    /**
     * Write an unsigned 32-bit integer in little-endian byte order.
     *
     * @param[in] v  Value to write.
     */
    void U32(uint32_t v);

    /**
     * Write an unsigned 64-bit integer in little-endian byte order.
     *
     * @param[in] v  Value to write.
     */
    void U64(uint64_t v);

    ///@}

    /**
     * Write raw bytes without a length prefix.
     *
     * @param[in] data  Bytes to append.
     */
    void Raw(std::span<const uint8_t> data);

    /**
     * Write length-prefixed bytes (u32 length + data).
     *
     * @param[in] data  Bytes to write, preceded by their u32 length.
     */
    void Bytes(std::span<const uint8_t> data);

    /**
     * @return Read-only view of the buffer written so far.
     */
    [[nodiscard]] std::span<const uint8_t> data() const;

    /**
     * Move the internal buffer out.  Writer is left empty.
     *
     * @return The accumulated byte buffer.
     */
    [[nodiscard]] std::vector<uint8_t> Take();

private:
    std::vector<uint8_t> m_buf;
};

} // namespace wire
} // namespace grovedb

#endif // GROVEDB_UTIL_WIRE_WRITE_H

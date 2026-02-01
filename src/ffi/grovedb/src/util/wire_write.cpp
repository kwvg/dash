// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifdef HAVE_GROVEDB_CONFIG
#include <config/grovedb-config.h>
#endif

#include <util/wire_write.h>

#include <util/polyfill/std23.hpp>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace grovedb {
namespace wire {

Writer::Writer(size_t reserve_hint)
{
    m_buf.reserve(reserve_hint);
}

void Writer::U8(uint8_t v)
{
    m_buf.push_back(v);
}

void Writer::U16(uint16_t v)
{
    if constexpr (std::endian::native != std::endian::little) {
        v = std23::byteswap(v);
    }
    auto bytes = std::bit_cast<std::array<uint8_t, 2>>(v);
    m_buf.insert(m_buf.end(), bytes.begin(), bytes.end());
}

void Writer::U32(uint32_t v)
{
    if constexpr (std::endian::native != std::endian::little) {
        v = std23::byteswap(v);
    }
    auto bytes = std::bit_cast<std::array<uint8_t, 4>>(v);
    m_buf.insert(m_buf.end(), bytes.begin(), bytes.end());
}

void Writer::U64(uint64_t v)
{
    if constexpr (std::endian::native != std::endian::little) {
        v = std23::byteswap(v);
    }
    auto bytes = std::bit_cast<std::array<uint8_t, 8>>(v);
    m_buf.insert(m_buf.end(), bytes.begin(), bytes.end());
}

void Writer::Raw(std::span<const uint8_t> data)
{
    m_buf.insert(m_buf.end(), data.begin(), data.end());
}

void Writer::Bytes(std::span<const uint8_t> data)
{
    U32(static_cast<uint32_t>(data.size()));
    m_buf.insert(m_buf.end(), data.begin(), data.end());
}

std::span<const uint8_t> Writer::data() const
{
    return m_buf;
}

std::vector<uint8_t> Writer::Take()
{
    return std::move(m_buf);
}

} // namespace wire
} // namespace grovedb

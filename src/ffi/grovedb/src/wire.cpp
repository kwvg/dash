// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifdef HAVE_GROVEDB_CONFIG
#include <config/grovedb-config.h>
#endif

#include <util/wire.h>

#include <util/polyfill/std23.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace grovedb {
namespace wire {

// ---------------------------------------------------------------------------
// Writer
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Reader
// ---------------------------------------------------------------------------

Reader::Reader(std::span<const uint8_t> data) : m_data{data} {}

Status Reader::U8(uint8_t& v)
{
    if (m_pos + 1 > m_data.size()) {
        return Status::Corruption("wire: buffer underflow reading u8");
    }
    v = m_data[m_pos];
    m_pos += 1;
    return Status::Ok();
}

Status Reader::U16(uint16_t& v)
{
    if (m_pos + 2 > m_data.size()) {
        return Status::Corruption("wire: buffer underflow reading u16");
    }
    std::array<uint8_t, 2> bytes;
    std::copy_n(m_data.data() + m_pos, 2, bytes.begin());
    v = std::bit_cast<uint16_t>(bytes);
    if constexpr (std::endian::native != std::endian::little) {
        v = std23::byteswap(v);
    }
    m_pos += 2;
    return Status::Ok();
}

Status Reader::U32(uint32_t& v)
{
    if (m_pos + 4 > m_data.size()) {
        return Status::Corruption("wire: buffer underflow reading u32");
    }
    std::array<uint8_t, 4> bytes;
    std::copy_n(m_data.data() + m_pos, 4, bytes.begin());
    v = std::bit_cast<uint32_t>(bytes);
    if constexpr (std::endian::native != std::endian::little) {
        v = std23::byteswap(v);
    }
    m_pos += 4;
    return Status::Ok();
}

Status Reader::U64(uint64_t& v)
{
    if (m_pos + 8 > m_data.size()) {
        return Status::Corruption("wire: buffer underflow reading u64");
    }
    std::array<uint8_t, 8> bytes;
    std::copy_n(m_data.data() + m_pos, 8, bytes.begin());
    v = std::bit_cast<uint64_t>(bytes);
    if constexpr (std::endian::native != std::endian::little) {
        v = std23::byteswap(v);
    }
    m_pos += 8;
    return Status::Ok();
}

Status Reader::Raw(std::span<uint8_t> out)
{
    if (m_pos + out.size() > m_data.size()) {
        return Status::Corruption("wire: buffer underflow reading raw bytes");
    }
    std::copy_n(m_data.data() + m_pos, out.size(), out.data());
    m_pos += out.size();
    return Status::Ok();
}

Status Reader::Bytes(grovedb::Bytes& out)
{
    uint32_t len{0};
    if (auto s = U32(len); !s.ok()) return s;
    if (m_pos + len > m_data.size()) {
        return Status::Corruption("wire: buffer underflow reading bytes");
    }
    out.assign(m_data.data() + m_pos, m_data.data() + m_pos + len);
    m_pos += len;
    return Status::Ok();
}

size_t Reader::remaining() const
{
    return m_data.size() - m_pos;
}

bool Reader::at_end() const
{
    return m_pos >= m_data.size();
}

} // namespace wire
} // namespace grovedb

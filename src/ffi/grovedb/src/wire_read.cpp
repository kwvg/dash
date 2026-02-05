// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <bit>

#include <grovedb/wire.h>

#include <util/assumptions.h>
#include <util/std23.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace grovedb {
namespace wire {
Reader::Reader(std::span<const uint8_t> data)
    : m_data{data}
{
}

Result<uint8_t, Error> Reader::U8()
{
  if (m_pos + 1 > m_data.size()) {
    return Err(Error::Corruption);
  }
  uint8_t v = m_data[m_pos];
  m_pos += 1;
  return v;
}

Result<uint16_t, Error> Reader::U16()
{
  if (m_pos + 2 > m_data.size()) {
    return Err(Error::Corruption);
  }
  std::array<uint8_t, 2> bytes;
  std::copy_n(m_data.data() + m_pos, 2, bytes.begin());
  uint16_t v = std::bit_cast<uint16_t>(bytes);
  if constexpr (std::endian::native != std::endian::little) {
    v = std23::byteswap(v);
  }
  m_pos += 2;
  return v;
}

Result<uint32_t, Error> Reader::U32()
{
  if (m_pos + 4 > m_data.size()) {
    return Err(Error::Corruption);
  }
  std::array<uint8_t, 4> bytes;
  std::copy_n(m_data.data() + m_pos, 4, bytes.begin());
  uint32_t v = std::bit_cast<uint32_t>(bytes);
  if constexpr (std::endian::native != std::endian::little) {
    v = std23::byteswap(v);
  }
  m_pos += 4;
  return v;
}

Result<uint64_t, Error> Reader::U64()
{
  if (m_pos + 8 > m_data.size()) {
    return Err(Error::Corruption);
  }
  std::array<uint8_t, 8> bytes;
  std::copy_n(m_data.data() + m_pos, 8, bytes.begin());
  uint64_t v = std::bit_cast<uint64_t>(bytes);
  if constexpr (std::endian::native != std::endian::little) {
    v = std23::byteswap(v);
  }
  m_pos += 8;
  return v;
}

Result<void, Error> Reader::Raw(std::span<uint8_t> out)
{
  if (m_pos + out.size() > m_data.size()) {
    return Err(Error::Corruption);
  }
  std::copy_n(m_data.data() + m_pos, out.size(), out.data());
  m_pos += out.size();
  return {};
}

Result<Bytes, Error> Reader::Bytes()
{
  return U32().and_then([this](uint32_t len) -> Result<grovedb::Bytes, Error> {
    // Safe overflow check: avoid m_pos + len which can wrap on 32-bit.
    if (len > m_data.size() || m_pos > m_data.size() - len) {
      return Err(Error::Corruption);
    }
    grovedb::Bytes out;
    out.assign(m_data.data() + m_pos, m_data.data() + m_pos + len);
    m_pos += len;
    return out;
  });
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

// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <grovedb/types.h>

#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <string_view>

namespace grovedb {
std::string Bytes::ToHex() const
{
  std::string hex;
  hex.reserve(size() * 2);
  for (auto b : *this) {
    hex += std::format("{:02x}", b);
  }
  return hex;
}

namespace {
std::optional<uint8_t> nibble(char c)
{
  if (c >= '0' && c <= '9') {
    return static_cast<uint8_t>(c - '0');
  }
  if (c >= 'a' && c <= 'f') {
    return static_cast<uint8_t>(c - 'a' + 10);
  }
  if (c >= 'A' && c <= 'F') {
    return static_cast<uint8_t>(c - 'A' + 10);
  }
  return std::nullopt;
}
} // anonymous namespace

Result<Bytes, Error> Bytes::FromHex(std::string_view sv)
{
  if (sv.size() % 2 != 0) {
    return Err(Error::InvalidArgument("FromHex: odd-length hex string"));
  }
  Bytes result;
  result.reserve(sv.size() / 2);
  for (size_t i = 0; i < sv.size(); i += 2) {
    auto hi = nibble(sv[i]);
    auto lo = nibble(sv[i + 1]);
    if (!hi || !lo) {
      return Err(Error::InvalidArgument("FromHex: invalid hex character"));
    }
    result.push_back(static_cast<uint8_t>((*hi << 4) | *lo));
  }
  return result;
}

std::string Hash::ToString() const
{
  std::string hex;
  hex.reserve(64);
  for (auto b : *this) {
    hex += std::format("{:02x}", b);
  }
  return hex;
}

Result<Hash, Error> Hash::FromHex(std::string_view sv)
{
  if (sv.size() != 64) {
    return Err(Error::InvalidArgument("Hash::FromHex: expected exactly 64 hex characters"));
  }
  Hash hash{};
  for (size_t i = 0; i < 32; ++i) {
    auto hi = nibble(sv[i * 2]);
    auto lo = nibble(sv[i * 2 + 1]);
    if (!hi || !lo) {
      return Err(Error::InvalidArgument("Hash::FromHex: invalid hex character"));
    }
    hash[i] = static_cast<uint8_t>((*hi << 4) | *lo);
  }
  return hash;
}
} // namespace grovedb

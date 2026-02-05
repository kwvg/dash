// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <grovedb/wire.h>

#include <boost/test/unit_test.hpp>

#include <array>
#include <cstdint>
#include <vector>

BOOST_AUTO_TEST_SUITE(wire_tests)

BOOST_AUTO_TEST_CASE(test_writer_reader_u8)
{
  grovedb::wire::Writer w;
  w.U8(0x00);
  w.U8(0x42);
  w.U8(0xFF);

  grovedb::wire::Reader r{w.data()};
  auto a = r.U8();
  auto b = r.U8();
  auto c = r.U8();
  BOOST_REQUIRE(a.has_value());
  BOOST_REQUIRE(b.has_value());
  BOOST_REQUIRE(c.has_value());
  BOOST_CHECK_EQUAL(*a, 0x00);
  BOOST_CHECK_EQUAL(*b, 0x42);
  BOOST_CHECK_EQUAL(*c, 0xFF);
  BOOST_CHECK(r.at_end());
}

BOOST_AUTO_TEST_CASE(test_writer_reader_u16)
{
  grovedb::wire::Writer w;
  w.U16(0x0000);
  w.U16(0x1234);
  w.U16(0xFFFF);

  // Verify LE byte order in raw buffer.
  auto buf = w.data();
  BOOST_CHECK_EQUAL(buf[2], 0x34); // low
  BOOST_CHECK_EQUAL(buf[3], 0x12); // high

  grovedb::wire::Reader r{buf};
  auto a = r.U16();
  auto b = r.U16();
  auto c = r.U16();
  BOOST_REQUIRE(a.has_value());
  BOOST_REQUIRE(b.has_value());
  BOOST_REQUIRE(c.has_value());
  BOOST_CHECK_EQUAL(*a, 0x0000);
  BOOST_CHECK_EQUAL(*b, 0x1234);
  BOOST_CHECK_EQUAL(*c, 0xFFFF);
  BOOST_CHECK(r.at_end());
}

BOOST_AUTO_TEST_CASE(test_writer_reader_u32)
{
  grovedb::wire::Writer w;
  w.U32(0x00000000);
  w.U32(0xDEADBEEF);
  w.U32(0xFFFFFFFF);

  // Verify LE byte order in raw buffer.
  auto buf = w.data();
  BOOST_CHECK_EQUAL(buf[4], 0xEF); // low byte of 0xDEADBEEF
  BOOST_CHECK_EQUAL(buf[5], 0xBE);
  BOOST_CHECK_EQUAL(buf[6], 0xAD);
  BOOST_CHECK_EQUAL(buf[7], 0xDE); // high byte

  grovedb::wire::Reader r{buf};
  auto a = r.U32();
  auto b = r.U32();
  auto c = r.U32();
  BOOST_REQUIRE(a.has_value());
  BOOST_REQUIRE(b.has_value());
  BOOST_REQUIRE(c.has_value());
  BOOST_CHECK_EQUAL(*a, 0x00000000);
  BOOST_CHECK_EQUAL(*b, 0xDEADBEEF);
  BOOST_CHECK_EQUAL(*c, 0xFFFFFFFF);
  BOOST_CHECK(r.at_end());
}

BOOST_AUTO_TEST_CASE(test_writer_reader_u64)
{
  grovedb::wire::Writer w;
  w.U64(0x0102030405060708);

  // Verify LE byte order.
  auto buf = w.data();
  BOOST_CHECK_EQUAL(buf[0], 0x08);
  BOOST_CHECK_EQUAL(buf[7], 0x01);

  grovedb::wire::Reader r{buf};
  auto v = r.U64();
  BOOST_REQUIRE(v.has_value());
  BOOST_CHECK_EQUAL(*v, 0x0102030405060708);
  BOOST_CHECK(r.at_end());
}

BOOST_AUTO_TEST_CASE(test_writer_reader_bytes)
{
  auto original = grovedb::Bytes::FromString("hello");
  grovedb::wire::Writer w;
  w.Bytes(original);

  // Buffer should be: [u32 len=5][h][e][l][l][o] = 9 bytes total.
  BOOST_CHECK_EQUAL(w.data().size(), 9);

  grovedb::wire::Reader r{w.data()};
  auto decoded = r.Bytes();
  BOOST_REQUIRE(decoded.has_value());
  BOOST_CHECK(*decoded == original);
  BOOST_CHECK(r.at_end());
}

BOOST_AUTO_TEST_CASE(test_writer_reader_raw)
{
  std::array<uint8_t, 4> original{0xCA, 0xFE, 0xBA, 0xBE};
  grovedb::wire::Writer w;
  w.Raw(original);

  BOOST_CHECK_EQUAL(w.data().size(), 4);

  grovedb::wire::Reader r{w.data()};
  std::array<uint8_t, 4> decoded{};
  auto result = r.Raw(decoded);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(decoded == original);
  BOOST_CHECK(r.at_end());
}

BOOST_AUTO_TEST_CASE(test_reader_underflow)
{
  // Empty buffer: reading anything should fail.
  std::vector<uint8_t> empty;
  grovedb::wire::Reader r{empty};

  auto u8_result = r.U8();
  BOOST_CHECK(!u8_result.has_value());
  BOOST_CHECK_EQUAL(u8_result.error(), grovedb::wire::Error::Corruption);

  // Partial buffer: 2 bytes is not enough for u32.
  std::vector<uint8_t> short_buf{0x01, 0x02};
  grovedb::wire::Reader r2{short_buf};

  auto u32_result = r2.U32();
  BOOST_CHECK(!u32_result.has_value());
  BOOST_CHECK_EQUAL(u32_result.error(), grovedb::wire::Error::Corruption);
}

BOOST_AUTO_TEST_CASE(test_encode_decode_convenience)
{
  auto original = grovedb::Bytes::FromString("test");
  auto buf = grovedb::wire::Encode(original);

  auto decoded = grovedb::wire::Decode<grovedb::Bytes>(std::span<const uint8_t>{buf});
  BOOST_REQUIRE(decoded.has_value());
  BOOST_CHECK(*decoded == original);
}

BOOST_AUTO_TEST_CASE(test_endianness_byte_order)
{
  // Write a known u32 and inspect the raw bytes for LE order.
  grovedb::wire::Writer w;
  w.U32(0x04030201);

  auto buf = w.data();
  BOOST_REQUIRE_EQUAL(buf.size(), 4);
  BOOST_CHECK_EQUAL(buf[0], 0x01);
  BOOST_CHECK_EQUAL(buf[1], 0x02);
  BOOST_CHECK_EQUAL(buf[2], 0x03);
  BOOST_CHECK_EQUAL(buf[3], 0x04);
}

// -- Path encoding/decoding round-trip tests --------------------------------

BOOST_AUTO_TEST_CASE(test_path_encode_decode_empty)
{
  grovedb::Path empty_path;
  auto buf = grovedb::wire::Encode(empty_path);

  // Empty path: just [u32 count=0] = 4 bytes.
  BOOST_CHECK_EQUAL(buf.size(), 4);

  auto decoded = grovedb::wire::Decode<grovedb::Path>(std::span<const uint8_t>{buf});
  BOOST_REQUIRE(decoded.has_value());
  BOOST_CHECK(decoded->empty());
}

BOOST_AUTO_TEST_CASE(test_path_encode_decode_single_segment)
{
  grovedb::Path path{grovedb::Bytes::FromString("abc")};
  auto buf = grovedb::wire::Encode(path);

  auto decoded = grovedb::wire::Decode<grovedb::Path>(std::span<const uint8_t>{buf});
  BOOST_REQUIRE(decoded.has_value());
  BOOST_REQUIRE_EQUAL(decoded->size(), 1);
  BOOST_CHECK((*decoded)[0] == path[0]);
}

BOOST_AUTO_TEST_CASE(test_path_encode_decode_multiple_segments)
{
  grovedb::Path path{
      grovedb::Bytes::FromString("ab"),
      grovedb::Bytes{},
      grovedb::Bytes::FromString("test"),
  };
  auto buf = grovedb::wire::Encode(path);

  auto decoded = grovedb::wire::Decode<grovedb::Path>(std::span<const uint8_t>{buf});
  BOOST_REQUIRE(decoded.has_value());
  BOOST_REQUIRE_EQUAL(decoded->size(), 3);
  BOOST_CHECK((*decoded)[0] == path[0]);
  BOOST_CHECK((*decoded)[1] == path[1]);
  BOOST_CHECK((*decoded)[2] == path[2]);
}

BOOST_AUTO_TEST_CASE(test_path_decode_truncated)
{
  // Only 2 bytes, not enough for a u32 count.
  std::vector<uint8_t> short_buf{0x01, 0x02};
  auto result = grovedb::wire::Decode<grovedb::Path>(std::span<const uint8_t>{short_buf});
  BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(path_segment_boundary)
{
  // Build a Path with exactly 256 segments (MAX_PATH_SEGMENTS).
  grovedb::Path max_path;
  max_path.reserve(grovedb::wire::MAX_PATH_SEGMENTS);
  for (uint32_t i = 0; i < grovedb::wire::MAX_PATH_SEGMENTS; ++i) {
    max_path.push_back(grovedb::Bytes{static_cast<uint8_t>(i & 0xFF)});
  }

  // Encode + decode should succeed at the boundary.
  auto buf = grovedb::wire::Encode(max_path);
  auto decoded = grovedb::wire::Decode<grovedb::Path>(std::span<const uint8_t>{buf});
  BOOST_REQUIRE(decoded.has_value());
  BOOST_CHECK_EQUAL(decoded->size(), grovedb::wire::MAX_PATH_SEGMENTS);
  BOOST_CHECK((*decoded)[0] == max_path[0]);
  BOOST_CHECK((*decoded)[255] == max_path[255]);

  // Build a Path with 257 segments (one over the limit).
  grovedb::Path over_path = max_path;
  over_path.push_back(grovedb::Bytes{'x'});

  // Encoding succeeds (Writer has no limit check).
  auto over_buf = grovedb::wire::Encode(over_path);

  // Decoding should fail with InvalidArgument.
  auto over_decoded = grovedb::wire::Decode<grovedb::Path>(std::span<const uint8_t>{over_buf});
  BOOST_CHECK(!over_decoded.has_value());
  BOOST_CHECK_EQUAL(over_decoded.error(), grovedb::wire::Error::InvalidArgument);
}

BOOST_AUTO_TEST_SUITE_END()

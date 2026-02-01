// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <grovedb/wire.h>

#include <array>
#include <cstdint>
#include <vector>

BOOST_AUTO_TEST_SUITE(wire_tests)

// ---------------------------------------------------------------------------
// Writer/Reader primitive roundtrip tests
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_writer_reader_u8)
{
    grovedb::wire::Writer w;
    w.U8(0x00);
    w.U8(0x42);
    w.U8(0xFF);

    grovedb::wire::Reader r{w.data()};
    uint8_t a{0}, b{0}, c{0};
    BOOST_REQUIRE(r.U8(a).ok());
    BOOST_REQUIRE(r.U8(b).ok());
    BOOST_REQUIRE(r.U8(c).ok());
    BOOST_CHECK_EQUAL(a, 0x00);
    BOOST_CHECK_EQUAL(b, 0x42);
    BOOST_CHECK_EQUAL(c, 0xFF);
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
    BOOST_CHECK_EQUAL(buf[2], 0x34); // low byte of 0x1234
    BOOST_CHECK_EQUAL(buf[3], 0x12); // high byte of 0x1234

    grovedb::wire::Reader r{buf};
    uint16_t a{0}, b{0}, c{0};
    BOOST_REQUIRE(r.U16(a).ok());
    BOOST_REQUIRE(r.U16(b).ok());
    BOOST_REQUIRE(r.U16(c).ok());
    BOOST_CHECK_EQUAL(a, 0x0000);
    BOOST_CHECK_EQUAL(b, 0x1234);
    BOOST_CHECK_EQUAL(c, 0xFFFF);
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
    uint32_t a{0}, b{0}, c{0};
    BOOST_REQUIRE(r.U32(a).ok());
    BOOST_REQUIRE(r.U32(b).ok());
    BOOST_REQUIRE(r.U32(c).ok());
    BOOST_CHECK_EQUAL(a, 0x00000000);
    BOOST_CHECK_EQUAL(b, 0xDEADBEEF);
    BOOST_CHECK_EQUAL(c, 0xFFFFFFFF);
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
    uint64_t v{0};
    BOOST_REQUIRE(r.U64(v).ok());
    BOOST_CHECK_EQUAL(v, 0x0102030405060708);
    BOOST_CHECK(r.at_end());
}

BOOST_AUTO_TEST_CASE(test_writer_reader_bytes)
{
    grovedb::Bytes original{'h', 'e', 'l', 'l', 'o'};
    grovedb::wire::Writer w;
    w.Bytes(original);

    // Buffer should be: [u32 len=5][h][e][l][l][o] = 9 bytes total.
    BOOST_CHECK_EQUAL(w.data().size(), 9);

    grovedb::wire::Reader r{w.data()};
    grovedb::Bytes decoded;
    BOOST_REQUIRE(r.Bytes(decoded).ok());
    BOOST_CHECK(decoded == original);
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
    BOOST_REQUIRE(r.Raw(decoded).ok());
    BOOST_CHECK(decoded == original);
    BOOST_CHECK(r.at_end());
}

// ---------------------------------------------------------------------------
// Compound type roundtrip tests
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_path_roundtrip)
{
    grovedb::Path original{
        {'r', 'o', 'o', 't'},
        {'c', 'h', 'i', 'l', 'd'},
        {'l', 'e', 'a', 'f'}};

    auto buf = grovedb::wire::Encode(original);

    grovedb::Path decoded;
    BOOST_REQUIRE(grovedb::wire::Decode(std::span<const uint8_t>{buf}, decoded).ok());
    BOOST_CHECK_EQUAL(decoded.size(), original.size());
    for (size_t i{0}; i < original.size(); ++i) {
        BOOST_CHECK(decoded[i] == original[i]);
    }
}

BOOST_AUTO_TEST_CASE(test_query_item_roundtrip)
{
    // Test all 10 QueryItem kinds.
    std::vector<grovedb::QueryItem> items{
        grovedb::QueryItem::Key({'k'}),
        grovedb::QueryItem::Range({'a'}, {'z'}),
        grovedb::QueryItem::RangeInclusive({'a'}, {'z'}),
        grovedb::QueryItem::RangeFull(),
        grovedb::QueryItem::RangeFrom({'m'}),
        grovedb::QueryItem::RangeTo({'m'}),
        grovedb::QueryItem::RangeToInclusive({'m'}),
        grovedb::QueryItem::RangeAfter({'m'}),
        grovedb::QueryItem::RangeAfterTo({'a'}, {'z'}),
        grovedb::QueryItem::RangeAfterToInclusive({'a'}, {'z'}),
    };

    for (const auto& original : items) {
        grovedb::wire::Writer w;
        grovedb::wire::WireWrite(w, original);

        grovedb::wire::Reader r{w.data()};
        grovedb::QueryItem decoded;
        BOOST_REQUIRE_MESSAGE(
            grovedb::wire::WireRead(r, decoded).ok(),
            "Failed to decode QueryItem kind " << static_cast<int>(original.kind()));
        BOOST_CHECK_EQUAL(decoded.kind(), original.kind());
        BOOST_CHECK(decoded.first() == original.first());
        BOOST_CHECK(decoded.second() == original.second());
        BOOST_CHECK(r.at_end());
    }
}

BOOST_AUTO_TEST_CASE(test_vector_roundtrip)
{
    std::vector<grovedb::Bytes> original{
        {'a', 'b', 'c'},
        {},
        {'x', 'y'},
    };

    auto buf = grovedb::wire::Encode(original);

    std::vector<grovedb::Bytes> decoded;
    BOOST_REQUIRE(grovedb::wire::Decode(std::span<const uint8_t>{buf}, decoded).ok());
    BOOST_CHECK_EQUAL(decoded.size(), original.size());
    for (size_t i{0}; i < original.size(); ++i) {
        BOOST_CHECK(decoded[i] == original[i]);
    }
}

// ---------------------------------------------------------------------------
// Error handling tests
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_reader_underflow)
{
    // Empty buffer: reading anything should fail.
    std::vector<uint8_t> empty;
    grovedb::wire::Reader r{empty};

    uint8_t u8{0};
    BOOST_CHECK(!r.U8(u8).ok());
    BOOST_CHECK_EQUAL(r.U8(u8).code(), grovedb::Status::kCorruption);

    // Partial buffer: 2 bytes is not enough for u32.
    std::vector<uint8_t> short_buf{0x01, 0x02};
    grovedb::wire::Reader r2{short_buf};

    uint32_t u32{0};
    BOOST_CHECK(!r2.U32(u32).ok());
    BOOST_CHECK_EQUAL(r2.U32(u32).code(), grovedb::Status::kCorruption);
}

// ---------------------------------------------------------------------------
// Convenience function tests
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_encode_decode_convenience)
{
    grovedb::Bytes original{'t', 'e', 's', 't'};
    auto buf = grovedb::wire::Encode(original);

    grovedb::Bytes decoded;
    BOOST_REQUIRE(grovedb::wire::Decode(std::span<const uint8_t>{buf}, decoded).ok());
    BOOST_CHECK(decoded == original);
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

BOOST_AUTO_TEST_SUITE_END()

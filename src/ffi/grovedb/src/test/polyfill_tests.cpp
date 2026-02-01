// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <util/polyfill/std23.hpp>

#include <cstdint>

BOOST_AUTO_TEST_SUITE(polyfill_tests)

BOOST_AUTO_TEST_CASE(test_byteswap_u16)
{
    BOOST_CHECK_EQUAL(std23::byteswap(uint16_t{0x0102}), uint16_t{0x0201});
    BOOST_CHECK_EQUAL(std23::byteswap(uint16_t{0x0000}), uint16_t{0x0000});
    BOOST_CHECK_EQUAL(std23::byteswap(uint16_t{0xFFFF}), uint16_t{0xFFFF});
}

BOOST_AUTO_TEST_CASE(test_byteswap_u32)
{
    BOOST_CHECK_EQUAL(std23::byteswap(uint32_t{0x01020304}), uint32_t{0x04030201});
    BOOST_CHECK_EQUAL(std23::byteswap(uint32_t{0x00000000}), uint32_t{0x00000000});
    BOOST_CHECK_EQUAL(std23::byteswap(uint32_t{0xFFFFFFFF}), uint32_t{0xFFFFFFFF});
}

BOOST_AUTO_TEST_CASE(test_byteswap_u64)
{
    BOOST_CHECK_EQUAL(
        std23::byteswap(uint64_t{0x0102030405060708}),
        uint64_t{0x0807060504030201});
    // Roundtrip: swap twice should return the original.
    uint64_t original{0xDEADBEEFCAFEBABE};
    BOOST_CHECK_EQUAL(std23::byteswap(std23::byteswap(original)), original);
}

BOOST_AUTO_TEST_CASE(test_byteswap_constexpr)
{
    // Verify constexpr evaluation.
    static_assert(std23::byteswap(uint16_t{0x0102}) == uint16_t{0x0201});
    static_assert(std23::byteswap(uint32_t{0x01020304}) == uint32_t{0x04030201});
    static_assert(std23::byteswap(uint64_t{0x0102030405060708}) == uint64_t{0x0807060504030201});
    BOOST_CHECK(true); // Test body required.
}

BOOST_AUTO_TEST_SUITE_END()

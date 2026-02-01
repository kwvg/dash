// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GROVEDB_UTIL_POLYFILL_STD23_HPP
#define GROVEDB_UTIL_POLYFILL_STD23_HPP

#include <cstdint>

#if __has_include(<version>)
#include <version>
#endif

// Drop-in polyfill for C++23 <bit> additions.
// Calling conventions mirror std:: exactly so that raising the library
// requirement later is a mechanical s/std23/std/g replacement.

namespace std23 {

#if defined(__cpp_lib_byteswap) && __cpp_lib_byteswap >= 202110L

using std::byteswap;

#else

/** Reverse the bytes of a 16-bit integer. */
constexpr uint16_t byteswap(uint16_t v)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap16(v);
#else
    return static_cast<uint16_t>((v >> 8) | (v << 8));
#endif
}

/** Reverse the bytes of a 32-bit integer. */
constexpr uint32_t byteswap(uint32_t v)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap32(v);
#else
    return (v >> 24)
         | ((v >> 8) & 0x0000FF00u)
         | ((v << 8) & 0x00FF0000u)
         | (v << 24);
#endif
}

/** Reverse the bytes of a 64-bit integer. */
constexpr uint64_t byteswap(uint64_t v)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap64(v);
#else
    return (v >> 56)
         | ((v >> 40) & 0x000000000000FF00ull)
         | ((v >> 24) & 0x0000000000FF0000ull)
         | ((v >>  8) & 0x00000000FF000000ull)
         | ((v <<  8) & 0x000000FF00000000ull)
         | ((v << 24) & 0x0000FF0000000000ull)
         | ((v << 40) & 0x00FF000000000000ull)
         | (v << 56);
#endif
}

#endif // __cpp_lib_byteswap

} // namespace std23

#endif // GROVEDB_UTIL_POLYFILL_STD23_HPP

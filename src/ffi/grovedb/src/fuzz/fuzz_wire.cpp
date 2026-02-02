// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// FUZZ TARGET: fuzz_wire
// Pure fuzz: wire format deserialization with adversarial input.
//

#include <grovedb/wire.h>

#include <cstdint>
#include <span>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    std::span<const uint8_t> input{data, size};

    // Try reading primitive types -- all should return Status (no crash).
    {
        grovedb::wire::Reader r{input};
        uint8_t v8{0};
        (void)r.U8(v8);
    }
    {
        grovedb::wire::Reader r{input};
        uint16_t v16{0};
        (void)r.U16(v16);
    }
    {
        grovedb::wire::Reader r{input};
        uint32_t v32{0};
        (void)r.U32(v32);
    }
    {
        grovedb::wire::Reader r{input};
        uint64_t v64{0};
        (void)r.U64(v64);
    }

    // Try reading length-prefixed Bytes.
    {
        grovedb::wire::Reader r{input};
        grovedb::Bytes bytes;
        (void)r.Bytes(bytes);
    }

    // Try reading a Path (vector of byte segments).
    {
        grovedb::wire::Reader r{input};
        grovedb::Path path;
        (void)grovedb::wire::WireRead(r, path);
    }

    // Try reading a QueryItem.
    {
        grovedb::wire::Reader r{input};
        grovedb::QueryItem qi;
        (void)grovedb::wire::WireRead(r, qi);
    }

    return 0;
}

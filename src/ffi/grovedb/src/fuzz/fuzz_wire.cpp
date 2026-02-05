// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <grovedb/wire.h>

#include <cstdint>
#include <span>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  std::span<const uint8_t> input{data, size};
  {
    grovedb::wire::Reader r{input};
    (void)r.U8();
  }
  {
    grovedb::wire::Reader r{input};
    (void)r.U16();
  }
  {
    grovedb::wire::Reader r{input};
    (void)r.U32();
  }
  {
    grovedb::wire::Reader r{input};
    (void)r.U64();
  }
  {
    grovedb::wire::Reader r{input};
    (void)r.Bytes();
  }
  return 0;
}

// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <FuzzedDataProvider.h>

#include <grovedb/element.h>
#include <grovedb/error.h>
#include <grovedb/types.h>
#include <grovedb/wire.h>

#include <fuzz/utils/check.h>
#include <fuzz/utils/grovedb.h>

#include <cstdint>
#include <span>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  FuzzedDataProvider fdp(data, size);

  // -- Exercise element factories with fuzzed inputs. -----------------------

  // Item with random bytes.
  {
    auto val = grovedb::fuzz::ConsumeValue(fdp);
    auto result = grovedb::Element::Item(val);
    if (result.has_value()) {
      CHECK_TRUE(!result->empty());
      CHECK_TRUE(!result->data().empty());
    }
  }

  // EmptyTree (no input needed, always succeeds).
  {
    auto result = grovedb::Element::EmptyTree();
    CHECK_OK(result);
    CHECK_TRUE(!result->empty());
  }

  // EmptySumTree (no input needed, always succeeds).
  {
    auto result = grovedb::Element::EmptySumTree();
    CHECK_OK(result);
    CHECK_TRUE(!result->empty());
  }

  // SumItem with random int64.
  {
    auto val = fdp.ConsumeIntegral<int64_t>();
    auto result = grovedb::Element::SumItem(val);
    if (result.has_value()) {
      CHECK_TRUE(!result->empty());
    }
  }

  // -- Exercise ConsumeElement helper. --------------------------------------
  {
    auto elem = grovedb::fuzz::ConsumeElement(fdp);
    // elem may be empty (default-constructed) if a factory failed.
    (void)elem.empty();
    (void)elem.data();
  }

  // -- Exercise Path wire encoding/decoding round-trip with fuzzed data. ----
  {
    auto path = grovedb::fuzz::ConsumePath(fdp);
    auto buf = grovedb::wire::Encode(path);
    auto decoded = grovedb::wire::Decode<grovedb::Path>(std::span<const uint8_t>{buf});
    CHECK_OK(decoded);
    CHECK_EQ(decoded->size(), path.size());
    for (size_t i = 0; i < path.size(); ++i) {
      CHECK_TRUE((*decoded)[i] == path[i]);
    }
  }

  // -- Exercise Path decoding of raw fuzz input (should not crash). ---------
  {
    auto remaining = fdp.ConsumeRemainingBytes<uint8_t>();
    auto result = grovedb::wire::Decode<grovedb::Path>(std::span<const uint8_t>{remaining});
    // May or may not succeed — we just check it doesn't crash.
    (void)result;
  }

  return 0;
}

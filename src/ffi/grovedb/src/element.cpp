// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <db_internal.h>
#include <util/ffi.h>

#include <grovedb/element.h>

#include <rust/grovedb_cxx/lib.h>

namespace grovedb {
Result<Element, Error> Element::Item(const Bytes& value)
{
  return CallFFI([&]() -> Result<Element, Error> {
    auto bytes = grovedb_cxx::grovedb_element_item(ToSlice(value));
    return Element::FromData(Bytes{bytes.begin(), bytes.end()});
  });
}

Result<Element, Error> Element::EmptyTree()
{
  return CallFFI([&]() -> Result<Element, Error> {
    auto bytes = grovedb_cxx::grovedb_element_empty_tree();
    return Element::FromData(Bytes{bytes.begin(), bytes.end()});
  });
}

Result<Element, Error> Element::EmptySumTree()
{
  return CallFFI([&]() -> Result<Element, Error> {
    auto bytes = grovedb_cxx::grovedb_element_empty_sum_tree();
    return Element::FromData(Bytes{bytes.begin(), bytes.end()});
  });
}

Result<Element, Error> Element::SumItem(int64_t value)
{
  return CallFFI([&]() -> Result<Element, Error> {
    auto bytes = grovedb_cxx::grovedb_element_sum_item(value);
    return Element::FromData(Bytes{bytes.begin(), bytes.end()});
  });
}
} // namespace grovedb

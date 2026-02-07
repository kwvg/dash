// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <db_internal.h>
#include <grovedb/element.h>

#include <rust/grovedb_cxx/lib.h>

namespace grovedb {
Result<Element, Error> Element::Item(const Bytes& value)
{
  try {
    rust::Slice<const uint8_t> value_slice{value.data(), value.size()};
    auto bytes = grovedb_cxx::grovedb_element_item(value_slice);
    Element element;
    element.m_data.assign(bytes.begin(), bytes.end());
    return element;
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<Element, Error> Element::EmptyTree()
{
  try {
    auto bytes = grovedb_cxx::grovedb_element_empty_tree();
    Element element;
    element.m_data.assign(bytes.begin(), bytes.end());
    return element;
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<Element, Error> Element::EmptySumTree()
{
  try {
    auto bytes = grovedb_cxx::grovedb_element_empty_sum_tree();
    Element element;
    element.m_data.assign(bytes.begin(), bytes.end());
    return element;
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<Element, Error> Element::SumItem(int64_t value)
{
  try {
    auto bytes = grovedb_cxx::grovedb_element_sum_item(value);
    Element element;
    element.m_data.assign(bytes.begin(), bytes.end());
    return element;
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}
} // namespace grovedb

// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifdef HAVE_GROVEDB_CONFIG
#include <config/grovedb-config.h>
#endif

#include <grovedb/element.h>

#include <rust/grovedb_cxx/lib.h>

namespace grovedb {
Status Element::Item(const Bytes& value, Element& element)
{
    try {
        rust::Slice<const uint8_t> value_slice{value.data(), value.size()};
        auto bytes = grovedb_cxx::grovedb_element_item(value_slice);
        element.m_data.assign(bytes.begin(), bytes.end());
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Element::EmptyTree(Element& element)
{
    try {
        auto bytes = grovedb_cxx::grovedb_element_empty_tree();
        element.m_data.assign(bytes.begin(), bytes.end());
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}
} // namespace grovedb

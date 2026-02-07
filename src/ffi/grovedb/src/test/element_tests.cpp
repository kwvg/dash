// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <grovedb/element.h>
#include <grovedb/error.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <string>
#include <vector>

BOOST_AUTO_TEST_SUITE(element_tests)

BOOST_AUTO_TEST_CASE(item_from_bytes)
{
  auto value = grovedb::Bytes::FromString("hello");
  auto result = grovedb::Element::Item(value);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(!result->empty());
  BOOST_CHECK(!result->data().empty());
}

BOOST_AUTO_TEST_CASE(item_from_empty_bytes)
{
  grovedb::Bytes value{};
  auto result = grovedb::Element::Item(value);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(!result->empty());
}

BOOST_AUTO_TEST_CASE(empty_tree)
{
  auto result = grovedb::Element::EmptyTree();
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(!result->empty());
  BOOST_CHECK(!result->data().empty());
}

BOOST_AUTO_TEST_CASE(empty_sum_tree)
{
  auto result = grovedb::Element::EmptySumTree();
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(!result->empty());
  BOOST_CHECK(!result->data().empty());
}

BOOST_AUTO_TEST_CASE(sum_item)
{
  auto result = grovedb::Element::SumItem(42);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(!result->empty());
  BOOST_CHECK(!result->data().empty());
}

BOOST_AUTO_TEST_CASE(sum_item_negative)
{
  auto result = grovedb::Element::SumItem(-100);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(!result->empty());
}

BOOST_AUTO_TEST_CASE(sum_item_zero)
{
  auto result = grovedb::Element::SumItem(0);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(!result->empty());
}

BOOST_AUTO_TEST_CASE(default_element_is_empty)
{
  grovedb::Element e;
  BOOST_CHECK(e.empty());
  BOOST_CHECK(e.data().empty());
}

// -- ErrorCode ToString tests -----------------------------------------------

BOOST_AUTO_TEST_CASE(error_code_to_string)
{
  BOOST_CHECK_EQUAL(grovedb::ToString(grovedb::ErrorCode::Ok), "Ok");
  BOOST_CHECK_EQUAL(grovedb::ToString(grovedb::ErrorCode::NotFound), "NotFound");
  BOOST_CHECK_EQUAL(grovedb::ToString(grovedb::ErrorCode::Corruption), "Corruption");
  BOOST_CHECK_EQUAL(grovedb::ToString(grovedb::ErrorCode::InvalidArgument), "InvalidArgument");
  BOOST_CHECK_EQUAL(grovedb::ToString(grovedb::ErrorCode::IOError), "IOError");
  BOOST_CHECK_EQUAL(grovedb::ToString(grovedb::ErrorCode::NotSupported), "NotSupported");
  BOOST_CHECK_EQUAL(grovedb::ToString(grovedb::ErrorCode::Aborted), "Aborted");
}

BOOST_AUTO_TEST_CASE(error_factory_methods)
{
  auto e = grovedb::Error::NotFound("key missing");
  BOOST_CHECK_EQUAL(e.code(), grovedb::ErrorCode::NotFound);
  BOOST_CHECK(!e.ok());
  BOOST_CHECK_EQUAL(e.message(), "key missing");

  auto e2 = grovedb::Error::IOError("disk full");
  BOOST_CHECK_EQUAL(e2.code(), grovedb::ErrorCode::IOError);
  BOOST_CHECK_EQUAL(e2.message(), "disk full");
}

BOOST_AUTO_TEST_CASE(error_default_is_ok)
{
  grovedb::Error e;
  BOOST_CHECK(e.ok());
  BOOST_CHECK_EQUAL(e.code(), grovedb::ErrorCode::Ok);
  BOOST_CHECK(e.message().empty());
}

// -- ElementType ToString tests ---------------------------------------------

BOOST_AUTO_TEST_CASE(element_type_to_string)
{
  BOOST_CHECK_EQUAL(grovedb::ToString(grovedb::ElementType::Item), "Item");
  BOOST_CHECK_EQUAL(grovedb::ToString(grovedb::ElementType::Tree), "Tree");
  BOOST_CHECK_EQUAL(grovedb::ToString(grovedb::ElementType::SumTree), "SumTree");
  BOOST_CHECK_EQUAL(grovedb::ToString(grovedb::ElementType::SumItem), "SumItem");
}

BOOST_AUTO_TEST_SUITE_END()

// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <grovedb/grovedb.h>

#include <boost/test/unit_test.hpp>

#include <string>

BOOST_AUTO_TEST_SUITE(misc_tests)

BOOST_AUTO_TEST_CASE(whoami)
{
  auto result = grovedb::GetWhoami();
  BOOST_CHECK(!result.empty());
  BOOST_CHECK(result.find("grovedb") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

// Copyright (c) 2012-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rest/server.h>
#include <test/util/setup_common.h>

#include <drogon/HttpRequest.h>

#include <boost/test/unit_test.hpp>

#include <string>

BOOST_FIXTURE_TEST_SUITE(rest_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(test_query_string)
{
    std::string param;
    RESTResponseFormat rf;
    // No query string
    rf = ParseDataFormat(param, "/rest/endpoint/someresource.json");
    BOOST_CHECK_EQUAL(param, "/rest/endpoint/someresource");
    BOOST_CHECK_EQUAL(rf, RESTResponseFormat::JSON);

    // Query string with single parameter
    rf = ParseDataFormat(param, "/rest/endpoint/someresource.bin?p1=v1");
    BOOST_CHECK_EQUAL(param, "/rest/endpoint/someresource");
    BOOST_CHECK_EQUAL(rf, RESTResponseFormat::BINARY);

    // Query string with multiple parameters
    rf = ParseDataFormat(param, "/rest/endpoint/someresource.hex?p1=v1&p2=v2");
    BOOST_CHECK_EQUAL(param, "/rest/endpoint/someresource");
    BOOST_CHECK_EQUAL(rf, RESTResponseFormat::HEX);

    // Incorrectly formed query string will not be handled
    rf = ParseDataFormat(param, "/rest/endpoint/someresource.json&p1=v1");
    BOOST_CHECK_EQUAL(param, "/rest/endpoint/someresource.json&p1=v1");
    BOOST_CHECK_EQUAL(rf, RESTResponseFormat::UNDEF);

    // Omitted data format with query string should return UNDEF and hide query string
    rf = ParseDataFormat(param, "/rest/endpoint/someresource?p1=v1");
    BOOST_CHECK_EQUAL(param, "/rest/endpoint/someresource");
    BOOST_CHECK_EQUAL(rf, RESTResponseFormat::UNDEF);

    // Data format specified after query string
    rf = ParseDataFormat(param, "/rest/endpoint/someresource?p1=v1.json");
    BOOST_CHECK_EQUAL(param, "/rest/endpoint/someresource");
    BOOST_CHECK_EQUAL(rf, RESTResponseFormat::UNDEF);
}

BOOST_AUTO_TEST_CASE(test_parse_accept_format)
{
    constexpr std::array<std::pair<std::string_view, RESTResponseFormat>, 7> test_vals{{
        {"", RESTResponseFormat::JSON},
        {"*/*", RESTResponseFormat::JSON},
        {"application/json", RESTResponseFormat::JSON},
        {"application/octet-stream", RESTResponseFormat::BINARY},
        {"text/plain", RESTResponseFormat::HEX},
        {"text/html", RESTResponseFormat::UNDEF},
        {"text/html, application/json", RESTResponseFormat::JSON},
    }};

    for (const auto& [accept, res] : test_vals) {
        auto req{drogon::HttpRequest::newHttpRequest()};
        if (!accept.empty()) req->addHeader("Accept", std::string{accept});
        BOOST_CHECK_EQUAL(ParseAcceptFormat(req), res);
    }
}

BOOST_AUTO_TEST_SUITE_END()

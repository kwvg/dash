// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <netbase.h>
#include <streams.h>
#include <evo/netinfo.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(evo_netinfo, RegTestingSetup)

BOOST_AUTO_TEST_CASE(mnnetinfo_rules)
{
    // Validate AddEntry() rules enforcement
    const std::vector<std::pair</*input=*/std::string, /*expected_ret=*/NetInfoStatus>> vals {
        /* Valid IPv4 address, valid port */
        {"1.1.1.1:8888", NetInfoStatus::Success},
        /* Valid IPv4 address, no port */
        {"1.1.1.1", NetInfoStatus::Success},
        /* Valid IPv4 address, valid port */
        {"1.1.1.1:9999", NetInfoStatus::BadPort}, // Mainnet P2P port on non-mainnet
        /* Valid IPv4 address, valid port */
        {"0.0.0.0:8888", NetInfoStatus::BadInput}, // Valid IPv4 formatting but invalid IPv4 address
        /* Valid IPv4 address, invalid port */
        {"1.1.1.1:99999", NetInfoStatus::BadInput}, // Port greater than uint16_t max
        /* Valid IPv6 address, valid port */
        {"[2606:4700:4700::1111]:8888", NetInfoStatus::BadInput}, // Only IPv4 allowed
        /* Valid domain address, valid port */
        {"example.com:8888", NetInfoStatus::BadInput}, // Domains are not allowed
        /* Invalid address, no port */
        {"this should not work", NetInfoStatus::BadInput}, // Prohibited characters (' ')
        /* Invalid address, valid port */
        {"1..1.1.1:8888", NetInfoStatus::BadInput}, // Incorrectly formatted IPv4 address
        /* No address, valid port */
        {":8888", NetInfoStatus::BadInput}, // Missing address
    };
    for (const auto& [input, expected_ret] : vals) {
        MnNetInfo netInfo;
        BOOST_CHECK_EQUAL(netInfo.AddEntry(Purpose::CORE_P2P, input), expected_ret);
    }
}

bool CheckIfSerSame(const CService& lhs, const MnNetInfo& rhs)
{
    CHashWriter ss_lhs(SER_GETHASH, 0), ss_rhs(SER_GETHASH, 0);
    ss_lhs << lhs;
    ss_rhs << rhs;
    return ss_lhs.GetSHA256() == ss_rhs.GetSHA256();
}

BOOST_AUTO_TEST_CASE(cservice_compatible)
{
    // Empty values should be the same
    CService service;
    MnNetInfo netInfo;
    BOOST_CHECK(CheckIfSerSame(service, netInfo));
    BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::BadInput); // Empty values are invalid

    // Valid IPv4 address
    service = LookupNumeric("1.1.1.1", 1234);
    BOOST_CHECK_EQUAL(netInfo.AddEntry(Purpose::CORE_P2P, "1.1.1.1:1234"), NetInfoStatus::Success);
    BOOST_CHECK(CheckIfSerSame(service, netInfo));
    BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Success); // AddEntry() succeeded

    // Lookup() failure, MnNetInfo should remain empty if Lookup() failed
    service = CService(); netInfo.Clear();
    BOOST_CHECK_EQUAL(netInfo.AddEntry(Purpose::CORE_P2P, "example.com"), NetInfoStatus::BadInput); // Domain lookups are prohibited
    BOOST_CHECK(CheckIfSerSame(service, netInfo));
    BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::BadInput); // Empty values are invalid
}

BOOST_AUTO_TEST_SUITE_END()

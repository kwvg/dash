// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <chainparams.h>
#include <netbase.h>
#include <streams.h>
#include <evo/netinfo.h>

#include <boost/test/unit_test.hpp>

static const std::vector<
    std::tuple</*input=*/std::string, /*expected_ret_mn=*/NetInfoStatus, /*expected_ret_ext=*/NetInfoStatus>
> vals{
    // Address and port specified
    {"1.1.1.1:8888", NetInfoStatus::Success, NetInfoStatus::Success},
    // - Port should default to default P2P core with MnNetInfo
    // - Ports are no longer implied with ExtNetInfo
    {"1.1.1.1", NetInfoStatus::Success, NetInfoStatus::BadPort},
    // - MnNetInfo doesn't mind using port 0
    // - ExtNetInfo prohibits non-zero ports
    {"1.1.1.1:0", NetInfoStatus::Success, NetInfoStatus::BadPort},
    // - Mainnet P2P port on non-mainnet cause failure in MnNetInfo
    // - ExtNetInfo is indifferent to choice of port unless it's a bad port which mainnet P2P port isn't
    {"1.1.1.1:9999", NetInfoStatus::BadPort, NetInfoStatus::Success},
    // - Non-mainnet P2P port is allowed in MnNetInfo regardless of bad port status
    // - Port 22 (SSH) is below the privileged ports threshold (1023) and is therefore a bad port, disallowed in ExtNetInfo
    {"1.1.1.1:22", NetInfoStatus::Success, NetInfoStatus::BadPort},
    // Valid IPv4 formatting but invalid IPv4 address
    {"0.0.0.0:8888", NetInfoStatus::BadInput, NetInfoStatus::BadInput},
    // Port greater than uint16_t max
    {"1.1.1.1:99999", NetInfoStatus::BadInput, NetInfoStatus::BadInput},
    // - Non-IPv4 addresses are prohibited in MnNetInfo
    // - Any valid BIP155 address is allowed in ExtNetInfo
    {"[2606:4700:4700::1111]:8888", NetInfoStatus::BadInput, NetInfoStatus::Success},
    // Domains are not allowed
    {"example.com:8888", NetInfoStatus::BadInput, NetInfoStatus::BadInput},
    // Incorrectly formatted IPv4 address
    {"1..1.1.1:8888", NetInfoStatus::BadInput, NetInfoStatus::BadInput},
    // Missing address
    {":8888", NetInfoStatus::BadInput, NetInfoStatus::BadInput},
};

BOOST_FIXTURE_TEST_SUITE(evo_netinfo_tests, RegTestingSetup)

void ValidateGetEntries(const NetInfoList& entries, const size_t expected_size)
{
    BOOST_CHECK_EQUAL(entries.size(), expected_size);
    for (const NetInfoEntry& entry : entries) {
        BOOST_CHECK(entry.IsTriviallyValid());
    }
}

BOOST_AUTO_TEST_CASE(mnnetinfo_rules)
{
    for (const auto& [input, expected_ret, _] : vals) {
        MnNetInfo netInfo;
        BOOST_CHECK_EQUAL(netInfo.AddEntry(input), expected_ret);
        if (expected_ret != NetInfoStatus::Success) {
            // An empty MnNetInfo is considered malformed
            BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Malformed);
            BOOST_CHECK(netInfo.GetEntries().empty());
        } else {
            BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Success);
            ValidateGetEntries(netInfo.GetEntries(), /*expected_size=*/1);
        }
    }

    {
        // MnNetInfo only stores one value, overwriting prohibited
        MnNetInfo netInfo;
        BOOST_CHECK_EQUAL(netInfo.AddEntry("1.1.1.1:8888"), NetInfoStatus::Success);
        BOOST_CHECK_EQUAL(netInfo.AddEntry("1.1.1.2:8888"), NetInfoStatus::MaxLimit);
        ValidateGetEntries(netInfo.GetEntries(), /*expected_size=*/1);
    }
}

BOOST_AUTO_TEST_CASE(extnetinfo_rules)
{
    for (const auto& [input, _, expected_ret] : vals) {
        ExtNetInfo netInfo;
        BOOST_CHECK_EQUAL(netInfo.AddEntry(input), expected_ret);
        if (expected_ret != NetInfoStatus::Success) {
            // An empty ExtNetInfo is considered malformed
            BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Malformed);
            BOOST_CHECK(netInfo.GetEntries().empty());
        } else {
            BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Success);
            ValidateGetEntries(netInfo.GetEntries(), /*expected_size=*/1);
        }
    }

    {
        // ExtNetInfo can store up to 32 entries, check limit enforcement
        ExtNetInfo netInfo;
        const uint64_t rand{std::max(uint64_t{1}, GetRand(EXTNETINFO_ENTRIES_LIMIT))};
        for (size_t idx = 1; idx <= EXTNETINFO_ENTRIES_LIMIT; idx++) {
            auto fn = [&](){ return netInfo.AddEntry(strprintf("1.1.1.%d:%d", idx, 9999 + idx)); };
            BOOST_CHECK_EQUAL(fn(), NetInfoStatus::Success);
            if (rand == idx) {
                // Additionally check that attempting to add the same entry again fails
                BOOST_CHECK_EQUAL(fn(), NetInfoStatus::Duplicate);
            }
        }
        BOOST_CHECK_EQUAL(netInfo.AddEntry("1.1.1.33:10032"), NetInfoStatus::MaxLimit);
        ValidateGetEntries(netInfo.GetEntries(), /*expected_size=*/32);
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

    // Valid IPv4 address, valid port
    service = LookupNumeric("1.1.1.1", 1234);
    BOOST_CHECK_EQUAL(netInfo.AddEntry("1.1.1.1:1234"), NetInfoStatus::Success);
    BOOST_CHECK(CheckIfSerSame(service, netInfo));

    // Valid IPv4 address, default P2P port implied
    service = LookupNumeric("1.1.1.1", Params().GetDefaultPort()); netInfo.Clear();
    BOOST_CHECK_EQUAL(netInfo.AddEntry("1.1.1.1"), NetInfoStatus::Success);
    BOOST_CHECK(CheckIfSerSame(service, netInfo));

    // Lookup() failure (domains not allowed), MnNetInfo should remain empty if Lookup() failed
    service = CService(); netInfo.Clear();
    BOOST_CHECK_EQUAL(netInfo.AddEntry("example.com"), NetInfoStatus::BadInput);
    BOOST_CHECK(CheckIfSerSame(service, netInfo));

    // Validation failure (non-IPv4 not allowed), MnNetInfo should remain empty if ValidateService() failed
    service = CService(); netInfo.Clear();
    BOOST_CHECK_EQUAL(netInfo.AddEntry("[2606:4700:4700::1111]:1738"), NetInfoStatus::BadInput);
    BOOST_CHECK(CheckIfSerSame(service, netInfo));
}

BOOST_AUTO_TEST_SUITE_END()

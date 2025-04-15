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
    std::tuple<
        /*input=*/std::pair</*purpose=*/uint8_t, /*addr=*/std::string>,
        /*expected_ret_mn=*/NetInfoStatus,
        /*expected_ret_ext=*/NetInfoStatus
    >
> vals{
    // Address and port specified
    {{Purpose::CORE_P2P, "1.1.1.1:8888"}, NetInfoStatus::Success, NetInfoStatus::Success},
    // - Port should default to default P2P core with MnNetInfo
    // - Ports are no longer implied with ExtNetInfo
    {{Purpose::CORE_P2P, "1.1.1.1"}, NetInfoStatus::Success, NetInfoStatus::BadPort},
    // - MnNetInfo doesn't mind using port 0
    // - ExtNetInfo prohibits non-zero ports
    {{Purpose::CORE_P2P, "1.1.1.1:0"}, NetInfoStatus::Success, NetInfoStatus::BadPort},
    // - Mainnet P2P port on non-mainnet cause failure in MnNetInfo
    // - ExtNetInfo is indifferent to choice of port unless it's a bad port which mainnet P2P port isn't
    {{Purpose::CORE_P2P, "1.1.1.1:9999"}, NetInfoStatus::BadPort, NetInfoStatus::Success},
    // - Non-mainnet P2P port is allowed in MnNetInfo regardless of bad port status
    // - Port 22 (SSH) is below the privileged ports threshold (1023) and is therefore a bad port, disallowed in ExtNetInfo
    {{Purpose::CORE_P2P, "1.1.1.1:22"}, NetInfoStatus::Success, NetInfoStatus::BadPort},
    // Valid IPv4 formatting but invalid IPv4 address
    {{Purpose::CORE_P2P, "0.0.0.0:8888"}, NetInfoStatus::BadInput, NetInfoStatus::BadInput},
    // Port greater than uint16_t max
    {{Purpose::CORE_P2P, "1.1.1.1:99999"}, NetInfoStatus::BadInput, NetInfoStatus::BadInput},
    // - Non-IPv4 addresses are prohibited in MnNetInfo
    // - The first address must be IPv4 and therefore is not allowed in ExtNetInfo
    {{Purpose::CORE_P2P, "[2606:4700:4700::1111]:8888"}, NetInfoStatus::BadInput, NetInfoStatus::BadInput},
    // Domains are not allowed
    {{Purpose::CORE_P2P, "example.com:8888"}, NetInfoStatus::BadInput, NetInfoStatus::BadInput},
    // Incorrectly formatted IPv4 address
    {{Purpose::CORE_P2P, "1..1.1.1:8888"}, NetInfoStatus::BadInput, NetInfoStatus::BadInput},
    // Missing address
    {{Purpose::CORE_P2P, ":8888"}, NetInfoStatus::BadInput, NetInfoStatus::BadInput},
    // Bad purpose code
    {{64, "1.1.1.1:8888"}, NetInfoStatus::MaxLimit, NetInfoStatus::MaxLimit},
    // - MnNetInfo doesn't allow storing anything except a Core P2P address
    // - ExtNetInfo allows storing Platform P2P addresses
    {{Purpose::PLATFORM_P2P, "1.1.1.1:8888"}, NetInfoStatus::MaxLimit, NetInfoStatus::Success},
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
        const auto& [purpose, addr] = input;
        MnNetInfo netInfo;
        BOOST_CHECK_EQUAL(netInfo.AddEntry(purpose, addr), expected_ret);
        if (expected_ret != NetInfoStatus::Success) {
            // An empty MnNetInfo is considered malformed
            BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Malformed);
            BOOST_CHECK(!netInfo.HasEntries(purpose));
            BOOST_CHECK(netInfo.GetEntries().empty());
        } else {
            BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Success);
            BOOST_CHECK(netInfo.HasEntries(purpose));
            ValidateGetEntries(netInfo.GetEntries(), /*expected_size=*/1);
        }
    }

    {
        // MnNetInfo only stores one value, overwriting prohibited
        MnNetInfo netInfo;
        BOOST_CHECK_EQUAL(netInfo.AddEntry(Purpose::CORE_P2P, "1.1.1.1:8888"), NetInfoStatus::Success);
        BOOST_CHECK_EQUAL(netInfo.AddEntry(Purpose::CORE_P2P, "1.1.1.2:8888"), NetInfoStatus::MaxLimit);
        BOOST_CHECK(netInfo.HasEntries(Purpose::CORE_P2P));
        ValidateGetEntries(netInfo.GetEntries(), /*expected_size=*/1);
    }

    {
        // MnNetInfo only allows storing a Core P2P address
        MnNetInfo netInfo;
        for (const auto purpose : {Purpose::PLATFORM_HTTP, Purpose::PLATFORM_P2P}) {
            BOOST_CHECK_EQUAL(netInfo.AddEntry(purpose, "1.1.1.1:8888"), NetInfoStatus::MaxLimit);
            BOOST_CHECK(!netInfo.HasEntries(purpose));
        }
        BOOST_CHECK(netInfo.GetEntries().empty());
    }
}

BOOST_AUTO_TEST_CASE(extnetinfo_rules)
{
    for (const auto& [input, _, expected_ret] : vals) {
        const auto& [purpose, addr] = input;
        ExtNetInfo netInfo;
        BOOST_CHECK_EQUAL(netInfo.AddEntry(purpose, addr), expected_ret);
        if (expected_ret != NetInfoStatus::Success) {
            // An empty ExtNetInfo is considered malformed
            BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Malformed);
            BOOST_CHECK(!netInfo.HasEntries(purpose));
            BOOST_CHECK(netInfo.GetEntries().empty());
        } else {
            BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Success);
            BOOST_CHECK(netInfo.HasEntries(purpose));
            ValidateGetEntries(netInfo.GetEntries(), /*expected_size=*/1);
        }
    }

    {
        // ExtNetInfo can store up to 32 entries, check limit enforcement
        ExtNetInfo netInfo;
        const uint64_t rand{std::max(uint64_t{1}, GetRand(EXTNETINFO_ENTRIES_LIMIT))};
        for (size_t idx = 1; idx <= EXTNETINFO_ENTRIES_LIMIT; idx++) {
            auto fn = [&](){ return netInfo.AddEntry(Purpose::CORE_P2P, strprintf("1.1.1.%d:%d", idx, 9999 + idx)); };
            BOOST_CHECK_EQUAL(fn(), NetInfoStatus::Success);
            if (rand == idx) {
                // Additionally check that attempting to add the same entry again fails
                BOOST_CHECK_EQUAL(fn(), NetInfoStatus::Duplicate);
            }
        }
        BOOST_CHECK_EQUAL(netInfo.AddEntry(Purpose::CORE_P2P, "1.1.1.33:10032"), NetInfoStatus::MaxLimit);
        BOOST_CHECK(netInfo.HasEntries(Purpose::CORE_P2P));
        // The limit applies *per purpose code* and therefore wouldn't error if the address was for a different purpose
        BOOST_CHECK(netInfo.HasEntries(!Purpose::PLATFORM_P2P));
        BOOST_CHECK_EQUAL(netInfo.AddEntry(Purpose::PLATFORM_P2P, "1.1.1.33:10032"), NetInfoStatus::Success);
        BOOST_CHECK(netInfo.HasEntries(Purpose::PLATFORM_P2P));
        BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Success);
        // GetEntries() is a tally of all entries across all purpose codes
        ValidateGetEntries(netInfo.GetEntries(), /*expected_size=*/32 + 1);
    }

    {
        // ExtNetInfo allows storing non-IPv4 addresses if they aren't the first entry
        ExtNetInfo netInfo; uint16_t port{8888};
        for (const auto purpose : {Purpose::CORE_P2P, Purpose::PLATFORM_P2P}) {
            BOOST_CHECK_EQUAL(netInfo.AddEntry(purpose, strprintf("[2606:4700:4700::1111]:%d", port)), NetInfoStatus::BadInput);
            BOOST_CHECK_EQUAL(netInfo.AddEntry(purpose, strprintf("1.1.1.1:%d", port)), NetInfoStatus::Success);
            BOOST_CHECK_EQUAL(netInfo.AddEntry(purpose, strprintf("[2606:4700:4700::1111]:%d", port)), NetInfoStatus::Success);
            BOOST_CHECK(netInfo.HasEntries(purpose));
            port++;
        }

        // Unless it is for PLATFORM_HTTP, which can store any address right off the bat
        BOOST_CHECK_EQUAL(netInfo.AddEntry(Purpose::PLATFORM_HTTP, strprintf("[2606:4700:4700::1111]:%d", port)), NetInfoStatus::Success);
        BOOST_CHECK(netInfo.HasEntries(Purpose::PLATFORM_HTTP));
        BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Success);
        ValidateGetEntries(netInfo.GetEntries(), /*expected_size=*/2 + 2 + 1);
    }

    {
        // ExtNetInfo doesn't let you store duplicates even if they have a different purpose code
        ExtNetInfo netInfo;
        for (const auto& [purpose, retval] : std::vector<std::pair<uint8_t, NetInfoStatus>>{
            {Purpose::CORE_P2P, NetInfoStatus::Success},
            {Purpose::PLATFORM_P2P, NetInfoStatus::Duplicate}})
        {
            for (size_t idx = 1; idx <= 5; idx++) {
                BOOST_CHECK_EQUAL(netInfo.AddEntry(purpose, strprintf("1.1.1.%d:%d", idx, 9999 + idx)), retval);
            }
        }
        BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Success);
        BOOST_CHECK(netInfo.HasEntries(Purpose::CORE_P2P));
        BOOST_CHECK(netInfo.HasEntries(!Purpose::PLATFORM_P2P));
        ValidateGetEntries(netInfo.GetEntries(), /*expected_size=*/5);
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
    BOOST_CHECK_EQUAL(netInfo.AddEntry(Purpose::CORE_P2P, "1.1.1.1:1234"), NetInfoStatus::Success);
    BOOST_CHECK(CheckIfSerSame(service, netInfo));

    // Valid IPv4 address, default P2P port implied
    service = LookupNumeric("1.1.1.1", Params().GetDefaultPort()); netInfo.Clear();
    BOOST_CHECK_EQUAL(netInfo.AddEntry(Purpose::CORE_P2P, "1.1.1.1"), NetInfoStatus::Success);
    BOOST_CHECK(CheckIfSerSame(service, netInfo));

    // Lookup() failure (domains not allowed), MnNetInfo should remain empty if Lookup() failed
    service = CService(); netInfo.Clear();
    BOOST_CHECK_EQUAL(netInfo.AddEntry(Purpose::CORE_P2P, "example.com"), NetInfoStatus::BadInput);
    BOOST_CHECK(CheckIfSerSame(service, netInfo));

    // Validation failure (non-IPv4 not allowed), MnNetInfo should remain empty if ValidateService() failed
    service = CService(); netInfo.Clear();
    BOOST_CHECK_EQUAL(netInfo.AddEntry(Purpose::CORE_P2P, "[2606:4700:4700::1111]:1738"), NetInfoStatus::BadInput);
    BOOST_CHECK(CheckIfSerSame(service, netInfo));
}

BOOST_AUTO_TEST_SUITE_END()

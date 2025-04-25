// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <chainparams.h>
#include <evo/netinfo.h>
#include <netbase.h>
#include <streams.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

static const std::vector<
    std::tuple<
        /*input=*/std::pair</*purpose=*/uint8_t, /*addr=*/std::string>,
        /*expected_ret_mn=*/NetInfoStatus,
        /*expected_ret_ext=*/NetInfoStatus
    >
> addr_vals{
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
    // Domains are not allowed for Core P2P or Platform P2P
    {{Purpose::CORE_P2P, "example.com:8888"}, NetInfoStatus::BadInput, NetInfoStatus::BadInput},
    {{Purpose::PLATFORM_P2P, "example.com:8888"}, NetInfoStatus::MaxLimit, NetInfoStatus::BadInput},
    // - MnNetInfo doesn't allow storing anything except a Core P2P address
    // - ExtNetInfo can store Platform HTTP addresses *as domains*
    {{Purpose::PLATFORM_HTTP, "example.com:8888"}, NetInfoStatus::MaxLimit, NetInfoStatus::Success},
    // ExtNetInfo can store onion addresses but must have non-zero port
    {{Purpose::PLATFORM_HTTP, "pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion:0"}, NetInfoStatus::MaxLimit, NetInfoStatus::BadPort},
    // ExtNetInfo can store onion addresses
    {{Purpose::PLATFORM_HTTP, "pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion:8888"}, NetInfoStatus::MaxLimit, NetInfoStatus::Success},
    // ExtNetInfo can store I2P addresses as long as it uses port 0
    {{Purpose::PLATFORM_HTTP, "udhdrtrcetjm5sxzskjyr5ztpeszydbh4dpl3pl4utgqqw2v4jna.b32.i2p:0"}, NetInfoStatus::MaxLimit, NetInfoStatus::Success},
    // ExtNetInfo can store I2P addresses but non-zero ports are not allowed
    {{Purpose::PLATFORM_HTTP, "udhdrtrcetjm5sxzskjyr5ztpeszydbh4dpl3pl4utgqqw2v4jna.b32.i2p:8888"}, NetInfoStatus::MaxLimit, NetInfoStatus::BadPort},
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
    for (const auto& [input, expected_ret, _] : addr_vals) {
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

BOOST_AUTO_TEST_CASE(domainport_rules)
{
    static const std::vector<std::pair</*addr=*/std::string, /*retval=*/DomainPort::Status>> domain_vals{
        {"awa", DomainPort::Status::BadLen}, // 3 (characters in domain) < 4 (minimum length)
        {"meow", DomainPort::Status::BadDotless},
        {"cat.", DomainPort::Status::BadCharPos}, // no empty label (trailing delimiter)
        {".cat", DomainPort::Status::BadCharPos}, // no empty label (leading delimiter)
        {"a..dot..a", DomainPort::Status::BadLabelLen}, // no empty label (extra delimiters)
        {"meow's macbook pro.local", DomainPort::Status::BadChar}, // ' is not a valid character in domains
        {"-w-.me.ow", DomainPort::Status::BadLabelCharPos}, // trailing hyphens are not allowed
        // 64 (characters in label) > 63 (maximum limit)
        {"yeowwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwch.ow.ie", DomainPort::Status::BadLabelLen},
        // 278 (characters in domain) > 253 (maximum limit)
        {"Loremipsumdolorsitametconsecteturadipiscingelitseddoeiusmodtempor"
         "incididuntutlaboreetdoloremagnaaliquaUtenimadminimveniamquisnostrud"
         "exercitationullamcolaborisnisiutaliquipexeacommodoconsequatDuisaute"
         "iruredolorinreprehenderitinvoluptatevelitessecillumdoloreeufugiatnullapariat.ur", DomainPort::Status::BadLen},
        {"server-1.me.ow", DomainPort::Status::Success}
    };

    for (const auto& [addr, retval] : domain_vals) {
        DomainPort service; ExtNetInfo netInfo;
        BOOST_CHECK_EQUAL(service.Set(addr, 1234), retval);
        if (retval != DomainPort::Status::Success) {
            BOOST_CHECK_EQUAL(service.Validate(), DomainPort::Status::Malformed); // Empty values report as Malformed
            BOOST_CHECK_EQUAL(netInfo.AddEntry(Purpose::PLATFORM_HTTP, service.ToStringAddrPort()), NetInfoStatus::BadInput);
        } else {
            BOOST_CHECK_EQUAL(service.Validate(), DomainPort::Status::Success);
            BOOST_CHECK_EQUAL(netInfo.AddEntry(Purpose::PLATFORM_HTTP, service.ToStringAddrPort()), NetInfoStatus::Success);
        }
    }

    {
        // DomainPort requires non-zero ports
        DomainPort service; ExtNetInfo netInfo;
        BOOST_CHECK_EQUAL(service.Set("example.com", 0), DomainPort::Status::BadPort);
        BOOST_CHECK_EQUAL(service.Validate(), DomainPort::Status::Malformed);
        BOOST_CHECK_EQUAL(netInfo.AddEntry(Purpose::PLATFORM_HTTP, service.ToStringAddrPort()), NetInfoStatus::BadInput);
    }

    {
        // DomainPort stores the domain in lower-case
        DomainPort lhs, rhs;
        BOOST_CHECK_EQUAL(lhs.Set("example.com", 1738), DomainPort::Status::Success);
        BOOST_CHECK_EQUAL(rhs.Set(ToUpper("example.com"), 1738), DomainPort::Status::Success);
        BOOST_CHECK_EQUAL(lhs.ToStringAddr(), rhs.ToStringAddr());
        BOOST_CHECK(lhs == rhs);
    }
}

BOOST_AUTO_TEST_CASE(extnetinfo_rules)
{
    for (const auto& [input, _, expected_ret] : addr_vals) {
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

    {
        // ExtNetInfo has additional rules for domains
        const std::vector<std::pair</*input=*/std::string, /*expected_ret=*/NetInfoStatus>> test_vals{
            // Port 21 (FTP) is below the privileged ports threshold (1023), not allowed
            {"example.com:21", NetInfoStatus::BadPort},
            // Port 80 (HTTP) is below the privileged ports threshold (1023) but still allowed
            {"example.com:80", NetInfoStatus::Success},
            // Port 443 (HTTPS) is below the privileged ports threshold (1023) but still allowed
            {"example.com:443", NetInfoStatus::Success},
            // .local is a prohibited TLD
            {"meows-macbook-pro.local:7777", NetInfoStatus::BadInput},
        };
        for (const auto& [input, expected_ret] : test_vals) {
            ExtNetInfo netInfo;
            BOOST_CHECK_EQUAL(netInfo.AddEntry(Purpose::PLATFORM_HTTP, input), expected_ret);
            if (expected_ret != NetInfoStatus::Success) {
                // An empty ExtNetInfo is considered malformed
                BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Malformed);
                BOOST_CHECK(!netInfo.HasEntries(Purpose::PLATFORM_HTTP));
                BOOST_CHECK(netInfo.GetEntries().empty());
            } else {
                BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Success);
                BOOST_CHECK(netInfo.HasEntries(Purpose::PLATFORM_HTTP));
                ValidateGetEntries(netInfo.GetEntries(), /*expected_size=*/1);
            }
        }
    }

    {
        // ExtNetInfo can recognize CJDNS addresses
        ExtNetInfo netInfo;
        BOOST_CHECK_EQUAL(NISToString(netInfo.AddEntry(Purpose::PLATFORM_HTTP, "[fc00:3344:5566:7788:9900:aabb:ccdd:eeff]:1234")), NISToString(NetInfoStatus::Success));
        BOOST_CHECK_EQUAL(netInfo.Validate(), NetInfoStatus::Success);
        BOOST_CHECK(netInfo.HasEntries(Purpose::PLATFORM_HTTP));
        ValidateGetEntries(netInfo.GetEntries(), /*expected_size=*/1);
        BOOST_CHECK(netInfo.GetEntries().at(0).get().GetAddrPort().value().get().IsCJDNS());
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

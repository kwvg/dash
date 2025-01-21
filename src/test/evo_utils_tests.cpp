// Copyright (c) 2022-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <chainparams.h>
#include <masternode/address.h>
#include <validation.h>

#include <llmq/params.h>
#include <llmq/options.h>

#include <boost/test/unit_test.hpp>

/* TODO: rename this file and test to llmq_options_test */
BOOST_AUTO_TEST_SUITE(evo_utils_tests)

void Test(NodeContext& node)
{
    using namespace llmq;
    auto tip = node.chainman->ActiveTip();
    const auto& consensus_params = Params().GetConsensus();
    BOOST_CHECK_EQUAL(IsQuorumTypeEnabledInternal(consensus_params.llmqTypeDIP0024InstantSend, tip, false, false), false);
    BOOST_CHECK_EQUAL(IsQuorumTypeEnabledInternal(consensus_params.llmqTypeDIP0024InstantSend, tip, true, false), true);
    BOOST_CHECK_EQUAL(IsQuorumTypeEnabledInternal(consensus_params.llmqTypeDIP0024InstantSend, tip, true, true), true);
    BOOST_CHECK_EQUAL(IsQuorumTypeEnabledInternal(consensus_params.llmqTypeChainLocks, tip, false, false), true);
    BOOST_CHECK_EQUAL(IsQuorumTypeEnabledInternal(consensus_params.llmqTypeChainLocks, tip, true, false), true);
    BOOST_CHECK_EQUAL(IsQuorumTypeEnabledInternal(consensus_params.llmqTypeChainLocks, tip, true, true), true);
    BOOST_CHECK_EQUAL(IsQuorumTypeEnabledInternal(consensus_params.llmqTypePlatform, tip, false, false), Params().IsTestChain());
    BOOST_CHECK_EQUAL(IsQuorumTypeEnabledInternal(consensus_params.llmqTypePlatform, tip, true, false), Params().IsTestChain());
    BOOST_CHECK_EQUAL(IsQuorumTypeEnabledInternal(consensus_params.llmqTypePlatform, tip, true, true), Params().IsTestChain());
    BOOST_CHECK_EQUAL(IsQuorumTypeEnabledInternal(consensus_params.llmqTypeMnhf, tip, false, false), true);
    BOOST_CHECK_EQUAL(IsQuorumTypeEnabledInternal(consensus_params.llmqTypeMnhf, tip, true, false), true);
    BOOST_CHECK_EQUAL(IsQuorumTypeEnabledInternal(consensus_params.llmqTypeMnhf, tip, true, true), true);
}

BOOST_FIXTURE_TEST_CASE(utils_IsQuorumTypeEnabled_tests_regtest, RegTestingSetup)
{
    Test(m_node);
}

BOOST_FIXTURE_TEST_CASE(utils_IsQuorumTypeEnabled_tests_mainnet, TestingSetup)
{
    Test(m_node);
}

BOOST_FIXTURE_TEST_CASE(mnaddr_tests, RegTestingSetup)
{
    MnAddr::DecodeStatus good_addr_status;
    MnAddr good_addr{"rmn1qveyacpzn0yq7wsuqphc330vtsn0dxzt2a2dkpsmjql77ewjcvxvq4jgkv0", good_addr_status};
    BOOST_CHECK(good_addr.IsValid());
    BOOST_CHECK_EQUAL(good_addr_status, MnAddr::DecodeStatus::Success);
    BOOST_CHECK_EQUAL(good_addr.GetHash(), TaggedHash("Robert'); DROP TABLE Students;--").GetSHA256());

    const std::vector<std::pair<MnAddr::DecodeStatus, std::string>> bad_values{
        /* Valid data but encoded using bech32 (expected bech32m, got bech32) */
        { MnAddr::DecodeStatus::NotBech32m, "rmn1qveyacpzn0yq7wsuqphc330vtsn0dxzt2a2dkpsmjql77ewjcvxvqqwc6fd" },
        /* Valid data but uses wrong prefix (expected 'rmn', got 'tmn') */
        { MnAddr::DecodeStatus::HRPBad, "tmn1qveyacpzn0yq7wsuqphc330vtsn0dxzt2a2dkpsmjql77ewjcvxvqs6du8c" },
        /* Valid data but reports wrong version (expected 0, got 1) */
        { MnAddr::DecodeStatus::DataVersionBad, "rmn1pveyacpzn0yq7wsuqphc330vtsn0dxzt2a2dkpsmjql77ewjcvxvq2ecn33" },
        /* Invalid data, encodes nothing */
        { MnAddr::DecodeStatus::DataEmpty, "rmn1jxtzts" },
        /* Invalid data, encodes uint256::ZERO but one byte less */
        { MnAddr::DecodeStatus::DataSizeBad, "rmn1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq83fx6u" }
    };
    for (const auto& [expected_status, test_str] : bad_values) {
        MnAddr::DecodeStatus bad_addr_status;
        MnAddr bad_addr{test_str, bad_addr_status};
        // Should be marked as bad and for the right reason
        BOOST_CHECK(!bad_addr.IsValid());
        BOOST_CHECK_EQUAL(bad_addr_status, expected_status);
        // Shouldn't return any value that isn't empty
        BOOST_CHECK(bad_addr.GetAddress().empty());
        BOOST_CHECK_EQUAL(bad_addr.GetHash(), uint256::ZERO);
    }
}

BOOST_FIXTURE_TEST_CASE(netmninfo_tests, RegTestingSetup)
{
    // TODO: We should be implementing an interface (!!!)
    MnNetInfo networkInfo;

    // Check domain validation works as expected
    auto check_bad_domain_and_port = [&networkInfo](std::string expected_err, std::string domain, uint16_t port) -> void {
        auto err_opt{networkInfo.AddEntry(Purpose::PLATFORM_API, domain, port)};
        BOOST_CHECK(err_opt.has_value());
        BOOST_CHECK_EQUAL(err_opt.value(), expected_err);
    };
    const std::vector<std::pair</*expected_err=*/std::string, /*domain_str=*/std::string>> bad_domains{
        // 3 (characters in domain) < 4 (minimum length)
        { "bad domain length", "uwu" },
        // no dotless allowed
        { "prohibited dotless", "meow" },
        // no empty label (trailing delimiter)
        { "prohibited domain character position", "cat." },
        // no empty label (leading delimiter)
        { "prohibited domain character position", ".cat" },
        // no empty label (extra delimiters)
        { "bad label length", "a..dot..a" },
        // no empty label (leading delimiter, but also bad TLD), should catch empty label first
        { "prohibited domain character position", ".lan" },
        // ' is not a valid character in domains
        { "prohibited domain character", "meow's macbook pro.local" },
        // .local is not allowed, bad TLD
        { "prohibited tld", "meows-macbook-pro.local" },
        // $*@?# are not valid characters in domains
        { "prohibited domain character", "meow.go.8irfhj94w$*H@??#493#@" },
        // trailing hyphens are not allowed
        { "prohibited label character position", "-w-.me.ow" },
        // 64 (characters in label) > 63 (maximum limit)
        { "bad label length",
          "yeowwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwch.ow.ie" },
        // 306 (characters in domain) > 253 (maximum limit)
        {"bad domain length",
         "CatnipandsomeotherspeciesinthegenusNepetaisknownforitseffectsoncatbehaviourCatsrub"
         "ontheplantrollonthegroundpawatitlickitandchewitSomeleapaboutandpurrsomecryoutCatsdoit"
         "foraboutfivetofifteenminutesafterwhicholfactoryfatigueusuallysetsinThenepetalactone"
         "incatnipactsasafelineattractantafteritentersthecatsno.se" }
    };

    // we don't allow ports <1024
    check_bad_domain_and_port("bad port", "uwu", /*port=*/1);

    // test domain validation with bad domain names
    for (const auto& [expected_err, domain_str] : bad_domains) {
        check_bad_domain_and_port(expected_err, domain_str, /*port=*/25555);
    }

    auto check_good_domain_and_port = [&networkInfo](std::string domain, uint16_t port) {
        BOOST_CHECK(!networkInfo.AddEntry(Purpose::PLATFORM_API, domain, port).has_value());
        // Make sure that we don't get an empty entries list after inserting a new entry
        auto domains_opt{networkInfo.GetDomainPorts(Purpose::PLATFORM_API)};
        BOOST_CHECK(domains_opt.has_value());
        // Make sure we're in that list
        auto domains{domains_opt.value()};
        DomainPort dp{domain, port};
        BOOST_CHECK(std::find(domains.begin(), domains.end(), dp) != domains.end());
        // Make sure we can't add ourselves again
        auto err_opt{networkInfo.AddEntry(Purpose::PLATFORM_API, domain, port)};
        BOOST_CHECK(err_opt.has_value());
        BOOST_CHECK_EQUAL(err_opt.value(), "duplicate entry");
    };

    const std::vector<DomainPort> good_domains {
        // hyphens are allowed if used properly
        { "server-1.me.ow", 1443 },
        // even though <1024, excluded from bad ports list, allowed (HTTP)
        { "server-2.me.ow", 80 },
        // even though <1024, excluded from bad ports list, allowed (HTTPS)
        { "server-3.me.ow", 443 }
    };

    // test domain validation with bad domain names
    for (const auto& [domain_str, port] : good_domains) {
        check_good_domain_and_port(domain_str, port);
    }
}

BOOST_AUTO_TEST_SUITE_END()

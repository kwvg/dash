// Copyright (c) 2022-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <chainparams.h>
#include <evo/extended.h>
#include <netbase.h>
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

BOOST_FIXTURE_TEST_CASE(extended_mninfo_tests, RegTestingSetup)
{
    { // domain tests
        // TODO: We should be implementing an interface (!!!)
        MnNetInfo networkInfo;

        // Check domain validation works as expected
        auto check_bad_domain_and_port = [&networkInfo](MnNetStatus expected_err, std::string domain, uint16_t port) -> void {
            auto ret_code{networkInfo.AddEntry(Purpose::PLATFORM_API, {domain, port})};
            BOOST_CHECK_EQUAL(ret_code, expected_err);
        };
        const std::vector</*domain_str=*/std::string> bad_domains{
            // 3 (characters in domain) < 4 (minimum length)
            "uwu",
            // no dotless allowed
            "meow",
            // no empty label (trailing delimiter)
            "cat.",
            // no empty label (leading delimiter)
            ".cat",
            // no empty label (extra delimiters)
            "a..dot..a",
            // no empty label (leading delimiter, but also bad TLD), should catch empty label first
            ".lan",
            // ' is not a valid character in domains
            "meow's macbook pro.local",
            // .local is not allowed, bad TLD
            "meows-macbook-pro.local",
            // $*@?# are not valid characters in domains
            "meow.go.8irfhj94w$*H@??#493#@",
            // trailing hyphens are not allowed
            "-w-.me.ow",
            // 64 (characters in label) > 63 (maximum limit)
            "yeowwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwch.ow.ie",
            // 306 (characters in domain) > 253 (maximum limit)
            "CatnipandsomeotherspeciesinthegenusNepetaisknownforitseffectsoncatbehaviourCatsrub"
            "ontheplantrollonthegroundpawatitlickitandchewitSomeleapaboutandpurrsomecryoutCatsdoit"
            "foraboutfivetofifteenminutesafterwhicholfactoryfatigueusuallysetsinThenepetalactone"
            "incatnipactsasafelineattractantafteritentersthecatsno.se"
        };

        // we don't allow ports <1024
        check_bad_domain_and_port(MnNetStatus::BadPort, "uwu", /*port=*/1);
        // port 0 is not on the bad ports list but is still disallowed
        check_bad_domain_and_port(MnNetStatus::BadPort, "uwu", /*port=*/0);

        // test domain validation with bad domain names
        for (const auto& domain_str : bad_domains) {
            check_bad_domain_and_port(MnNetStatus::BadInput, domain_str, /*port=*/25555);
        }

        auto check_good_domain_and_port = [&networkInfo](std::string domain, uint16_t port) {
            DomainPort dp{domain, port};
            auto ret_code{networkInfo.AddEntry(Purpose::PLATFORM_API, dp)};
            BOOST_CHECK_EQUAL(ret_code, MnNetStatus::Success);
            // Make sure that we don't get an empty entries list after inserting a new entry
            auto domains{networkInfo.GetDomainPorts(Purpose::PLATFORM_API)};
            BOOST_CHECK(!domains.empty());
            // Make sure we can't add ourselves again
            auto err_code{networkInfo.AddEntry(Purpose::PLATFORM_API, dp)};
            BOOST_CHECK_EQUAL(err_code, MnNetStatus::Duplicate);
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
    } // end domain tests
    { // service tests
        MnNetInfo networkInfo;

        // ----------- test maximum entries limit -------------------
        CNetAddr netaddr{LookupHost("1.2.3.4", /*fAllowLookup=*/false).value()};
        // Populate maximum entries
        uint16_t port{9999}; // We need to increment the port so that we don't get told off for duplicate entries
        for (size_t idx = 0; idx <= MNADDR_ENTRIES_LIMIT; idx++) {
            BOOST_CHECK_EQUAL(networkInfo.AddEntry(Purpose::CORE_P2P, CService{netaddr, port}), MnNetStatus::Success);
            port++;
        }
        // Going over the limit is disallowed
        BOOST_CHECK_EQUAL(networkInfo.AddEntry(Purpose::CORE_P2P, CService{netaddr, port}), MnNetStatus::MaxLimit);
        // The limit doesn't carry over to another purpose's entry list
        BOOST_CHECK_EQUAL(networkInfo.AddEntry(Purpose::PLATFORM_P2P, CService{netaddr, port}), MnNetStatus::Success);
    } // end service tests

}

BOOST_AUTO_TEST_SUITE_END()

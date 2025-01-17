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

BOOST_AUTO_TEST_SUITE_END()

#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test network information fields across RPCs."""

from test_framework.util import (
    assert_equal
)
from test_framework.script import (
    hash160
)
from test_framework.test_framework import (
    BitcoinTestFramework,
    p2p_port,
    EVONODE_COLLATERAL,
    MASTERNODE_COLLATERAL
)
from test_framework.test_node import TestNode

from _decimal import Decimal
from random import randint

class Node:
    address_collateral: str = ""
    address_funds: str = ""
    address_owner: str = ""
    address_reward: str = ""
    address_voting: str = ""
    is_evo: bool = False
    collateral_txid: str = ""
    collateral_vout: int = 0
    node: TestNode
    operator_pk: str = ""
    operator_sk: str = ""
    platform_nodeid: str = ""
    provider_txid: str = ""

    def __init__(self, node: TestNode, is_evo: bool):
        self.is_evo = is_evo
        self.node = node
        self.generate_addresses()

    def bury_tx(self, test: BitcoinTestFramework, txid: str, depth: int):
        chain_tip = test.generate(self.node, depth)[0]
        assert_equal(self.node.getrawtransaction(txid, 1, chain_tip)['confirmations'], depth)

    def generate_addresses(self, force_all: bool = False):
        if not self.address_collateral or force_all:
            self.address_collateral = self.node.getnewaddress()
        if not self.address_funds or force_all:
            self.address_funds = self.node.getnewaddress()
        if not self.address_owner or force_all:
            self.address_owner = self.node.getnewaddress()
        if not self.address_reward or force_all:
            self.address_reward = self.node.getnewaddress()
        if not self.address_voting or force_all:
            self.address_voting = self.node.getnewaddress()
        if not self.operator_pk or not self.operator_sk or force_all:
            bls_ret = self.node.bls('generate')
            self.operator_pk = bls_ret['public']
            self.operator_sk = bls_ret['secret']

    def generate_collateral(self, test: BitcoinTestFramework):
        while self.node.getbalance() < self.get_collateral_value():
            test.bump_mocktime(1)
            test.generate(self.node, 10, sync_fun=test.no_op)

        self.collateral_txid = self.node.sendmany("", {self.address_collateral: self.get_collateral_value(), self.address_funds: 1})
        self.bury_tx(test, self.collateral_txid, 1)
        self.collateral_vout = self.get_vout_by_value(self.collateral_txid, Decimal(self.get_collateral_value()))
        assert self.collateral_vout != -1

    def get_collateral_value(self) -> int:
        return EVONODE_COLLATERAL if self.is_evo else MASTERNODE_COLLATERAL

    def get_vout_by_value(self, txid: str, value: Decimal) -> int:
        for txout in self.node.getrawtransaction(txid, 1)['vout']:
            if txout['value'] == value:
                return txout['n']
        return -1

    def is_mn_visible(self, _protx_hash = None) -> bool:
        protx_hash = _protx_hash or self.provider_txid
        mn_list = self.node.masternodelist()
        mn_visible = False
        for mn_entry in mn_list:
            dmn = mn_list.get(mn_entry)
            if dmn['proTxHash'] == protx_hash:
                assert_equal(dmn['type'], "Evo" if self.is_evo else "Regular")
                mn_visible = True
        return mn_visible

    def register_mn(self, test: BitcoinTestFramework, submit: bool, addrs_core_p2p, addrs_platform_http = None, addrs_platform_p2p = None) -> str:
        protx_output: str = ""
        if self.is_evo:
            assert(addrs_platform_http and addrs_platform_p2p)
            self.platform_nodeid = hash160(b'%d' % randint(1, 65535)).hex()
            protx_output = self.node.protx(
                "register_evo", self.collateral_txid, self.collateral_vout, addrs_core_p2p, self.address_owner, self.operator_pk,
                self.address_voting, 0, self.address_reward, self.platform_nodeid, addrs_platform_p2p, addrs_platform_http,
                self.address_funds, submit)
        else:
            protx_output = self.node.protx(
                "register", self.collateral_txid, self.collateral_vout, addrs_core_p2p, self.address_owner, self.operator_pk,
                self.address_voting, 0, self.address_reward, self.address_funds, submit)
        if submit:
            self.provider_txid = protx_output
            self.bury_tx(test, self.provider_txid, 1)
            assert_equal(self.is_mn_visible(), True)
            test.log.debug(f"Registered {'Evo' if self.is_evo else 'regular'} masternode with collateral_txid={self.collateral_txid}, "
                           f"collateral_vout={self.collateral_vout}, provider_txid={self.provider_txid}")
            test.restart_node(self.node.index, extra_args=self.node.extra_args + [f'-masternodeblsprivkey={self.operator_sk}'])
            return self.provider_txid
        return ""

    def update_mn(self, test: BitcoinTestFramework, addrs_core_p2p, addrs_platform_http = None, addrs_platform_p2p = None) -> str:
        update_txid: str = ""
        if self.is_evo:
            assert(addrs_platform_http and addrs_platform_p2p)
            update_txid = self.node.protx('update_service_evo', self.provider_txid, addrs_core_p2p, self.operator_sk, self.platform_nodeid,
                                          addrs_platform_p2p, addrs_platform_http, "", self.address_funds)
        else:
            update_txid = self.node.protx('update_service', self.provider_txid, addrs_core_p2p, self.operator_sk, "",
                                          self.address_funds)
        self.bury_tx(test, update_txid, 1)
        assert_equal(self.is_mn_visible(), True)
        test.log.debug(f"Updated {'Evo' if self.is_evo else 'regular'} masternode with collateral_txid={self.collateral_txid}, "
                       f"collateral_vout={self.collateral_vout}, provider_txid={self.provider_txid}")
        return update_txid

    def destroy_mn(self, test: BitcoinTestFramework):
        # Get UTXO from address used to pay fees
        address_funds_unspent = self.node.listunspent(0, 99999, [self.address_funds])[0]
        address_funds_value = address_funds_unspent['amount']

        # Reserve new address for collateral and fee spending
        new_address_collateral = self.node.getnewaddress()
        new_address_funds = self.node.getnewaddress()

        # Create transaction to spend old collateral and fee change
        raw_tx = self.node.createrawtransaction([
                { 'txid': self.collateral_txid, 'vout': self.collateral_vout },
                { 'txid': address_funds_unspent['txid'], 'vout': address_funds_unspent['vout'] }
            ], [
                {new_address_collateral: float(self.get_collateral_value())},
                {new_address_funds: float(address_funds_value - Decimal(0.001))}
            ])
        raw_tx = self.node.signrawtransactionwithwallet(raw_tx)['hex']

        # Send that transaction, resulting txid is new collateral
        new_collateral_txid = self.node.sendrawtransaction(raw_tx)
        self.bury_tx(test, new_collateral_txid, 1)
        new_collateral_vout = self.get_vout_by_value(new_collateral_txid, Decimal(self.get_collateral_value()))
        assert new_collateral_vout != -1

        # Old masternode entry should be dead
        assert_equal(self.is_mn_visible(self.provider_txid), False)
        test.log.debug(f"Destroyed {'Evo' if self.is_evo else 'regular'} masternode with collateral_txid={self.collateral_txid}, "
                       f"collateral_vout={self.collateral_vout}, provider_txid={self.provider_txid}")

        # Generate fresh addresses (and overwrite some of them with addresses used here)
        self.generate_addresses(True)
        self.address_collateral = new_address_collateral
        self.address_funds = new_address_funds
        self.collateral_txid = new_collateral_txid
        self.collateral_vout = new_collateral_vout
        self.provider_txid = ""

        # Restart node sans masternodeblsprivkey
        test.restart_node(self.node.index, extra_args=self.node.extra_args)

class NetInfoTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.extra_args = [
            ["-dip3params=2:2"],
            ["-deprecatedrpc=service", "-dip3params=2:2"],
            ["-dip3params=2:2"]
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def check_netinfo_fields(self, val, core_p2p_port: int, platform_http_port = None, platform_p2p_port = None):
        assert_equal(val['core_p2p'][0], f"127.0.0.1:{core_p2p_port}")
        if platform_http_port:
            assert_equal(val['platform_http'][0], f"127.0.0.1:{platform_http_port}")
        if platform_p2p_port:
            assert_equal(val['platform_p2p'][0], f"127.0.0.1:{platform_p2p_port}")

    # TODO: Cover CDeterministicMNStateDiff
    def run_test(self):
        node_evo: Node = Node(self.nodes[0], True)
        node_evo.generate_collateral(self)

        node_simple: TestNode = self.nodes[1]

        # netInfo is represented with JSON in CProRegTx, CProUpServTx, CDeterministicMNState and CSimplifiedMNListEntry,
        # so we need to test calls that rely on these underlying implementations. Start by collecting RPC responses.
        self.log.info("Collect JSON RPC responses from node")

        # CProRegTx::ToJson() <- TxToUniv() <- TxToJSON() <- getrawtransaction
        proregtx_hash = node_evo.register_mn(self, True, f"127.0.0.1:{p2p_port(node_evo.node.index)}", "19998", "29998")
        proregtx_rpc = node_evo.node.getrawtransaction(proregtx_hash, True)

        # CDeterministicMNState::ToJson() <- CDeterministicMN::pdmnState <- masternode_status
        masternode_status = node_evo.node.masternode('status')

        # Generate deprecation-disabled response to avoid having to re-create a masternode again later on
        self.restart_node(node_evo.node.index, extra_args=node_evo.node.extra_args +
                          [f'-masternodeblsprivkey={node_evo.operator_sk}', '-deprecatedrpc=service'])
        self.connect_nodes(node_evo.node.index, node_simple.index) # Needed as restarts don't reconnect nodes
        masternode_status_depr = node_evo.node.masternode('status')

        # Stop actively running the masternode so we can issue a CProUpServTx (and enable the deprecation)
        self.restart_node(node_evo.node.index, extra_args=node_evo.node.extra_args)
        self.connect_nodes(node_evo.node.index, node_simple.index) # Needed as restarts don't reconnect nodes

        # CProUpServTx::ToJson() <- TxToUniv() <- TxToJSON() <- getrawtransaction
        proupservtx_hash = node_evo.update_mn(self, f"127.0.0.1:{p2p_port(node_evo.node.index)}", "19998", "29998")
        proupservtx_rpc = node_evo.node.getrawtransaction(proupservtx_hash, True)

        # CSimplifiedMNListEntry::ToJson() <- CSimplifiedMNListDiff::mnList <- CSimplifiedMNListDiff::ToJson() <- protx_diff
        masternode_active_height: int = masternode_status['dmnState']['registeredHeight']
        protx_diff_rpc = node_evo.node.protx('diff', masternode_active_height - 1, masternode_active_height)

        self.log.info("Test RPCs return an 'addresses' field")
        assert "addresses" in proregtx_rpc['proRegTx'].keys()
        assert "addresses" in masternode_status['dmnState'].keys()
        assert "addresses" in proupservtx_rpc['proUpServTx'].keys()
        assert "addresses" in protx_diff_rpc['mnList'][0].keys()

        self.log.info("Test 'addresses' report for each purpose code correctly")
        self.check_netinfo_fields(proregtx_rpc['proRegTx']['addresses'], p2p_port(node_evo.node.index), "19998", "29998")
        self.check_netinfo_fields(masternode_status['dmnState']['addresses'], p2p_port(node_evo.node.index), "19998", "29998")
        self.check_netinfo_fields(proupservtx_rpc['proUpServTx']['addresses'], p2p_port(node_evo.node.index), "19998", "29998")
        # Note: CSimplifiedMNListEntry doesn't store 'platformP2PPort' at all
        self.check_netinfo_fields(protx_diff_rpc['mnList'][0]['addresses'], p2p_port(node_evo.node.index), "19998", platform_p2p_port=None)

        self.log.info("Test RPCs by default no longer return a 'service' field")
        assert "service" not in proregtx_rpc['proRegTx'].keys()
        assert "service" not in masternode_status['dmnState'].keys()
        assert "service" not in proupservtx_rpc['proUpServTx'].keys()
        assert "service" not in protx_diff_rpc['mnList'][0].keys()
        # "service" in "masternode status" is exempt from the deprecation as the primary address is
        # relevant on the host node as opposed to expressing payload information in most other RPCs.
        assert "service" in masternode_status.keys()

        self.log.info("Test RPCs by default no longer return a 'platformP2PPort' field")
        assert "platformP2PPort" not in proregtx_rpc['proRegTx'].keys()
        assert "platformP2PPort" not in masternode_status['dmnState'].keys()
        assert "platformP2PPort" not in proupservtx_rpc['proUpServTx'].keys()
        assert "platformP2PPort" not in protx_diff_rpc['mnList'][0].keys()

        self.log.info("Test RPCs by default no longer return a 'platformHTTPPort' field")
        assert "platformHTTPPort" not in proregtx_rpc['proRegTx'].keys()
        assert "platformHTTPPort" not in masternode_status['dmnState'].keys()
        assert "platformHTTPPort" not in proupservtx_rpc['proUpServTx'].keys()
        assert "platformHTTPPort" not in protx_diff_rpc['mnList'][0].keys()

        node_evo.destroy_mn(self) # Shut down previous masternode
        self.connect_nodes(node_evo.node.index, node_simple.index) # Needed as restarts don't reconnect nodes

        self.log.info("Collect RPC responses from node with -deprecatedrpc=service")

        # Re-use chain activity from earlier
        proregtx_rpc = node_simple.getrawtransaction(proregtx_hash, True)
        proupservtx_rpc = node_simple.getrawtransaction(proupservtx_hash, True)
        protx_diff_rpc = node_simple.protx('diff', masternode_active_height - 1, masternode_active_height)
        masternode_status = masternode_status_depr # Pull in response generated from earlier

        self.log.info("Test RPCs return 'addresses' even with -deprecatedrpc=service")
        assert "addresses" in proregtx_rpc['proRegTx'].keys()
        assert "addresses" in masternode_status['dmnState'].keys()
        assert "addresses" in proupservtx_rpc['proUpServTx'].keys()
        assert "addresses" in protx_diff_rpc['mnList'][0].keys()

        self.log.info("Test RPCs return 'service' with -deprecatedrpc=service")
        assert "service" in proregtx_rpc['proRegTx'].keys()
        assert "service" in masternode_status['dmnState'].keys()
        assert "service" in proupservtx_rpc['proUpServTx'].keys()
        assert "service" in protx_diff_rpc['mnList'][0].keys()

        self.log.info("Test RPCs return 'platformP2PPort' with -deprecatedrpc=service")
        assert "platformP2PPort" in proregtx_rpc['proRegTx'].keys()
        assert "platformP2PPort" in masternode_status['dmnState'].keys()
        assert "platformP2PPort" in proupservtx_rpc['proUpServTx'].keys()
        # CSimplifiedMNListEntry doesn't store 'platformP2PPort' at all
        assert "platformP2PPort" not in protx_diff_rpc['mnList'][0].keys()

        self.log.info("Test RPCs return 'platformHTTPPort' with -deprecatedrpc=service")
        assert "platformHTTPPort" in proregtx_rpc['proRegTx'].keys()
        assert "platformHTTPPort" in masternode_status['dmnState'].keys()
        assert "platformHTTPPort" in proupservtx_rpc['proUpServTx'].keys()
        assert "platformHTTPPort" in protx_diff_rpc['mnList'][0].keys()

if __name__ == "__main__":
    NetInfoTest().main()

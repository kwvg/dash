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

    def register_mn(self, test: BitcoinTestFramework, submit: bool, addrs_core_p2p, addrs_platform_http = None, addrs_platform_p2p = None):
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

    def update_mn(self, test: BitcoinTestFramework, addrs_core_p2p, addrs_platform_http = None, addrs_platform_p2p = None):
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

class NetInfoTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.extra_args = [
            ["-dip3params=2:2"],
            ["-dip3params=2:2"],
            ["-deprecatedrpc=service", "-dip3params=2:2"]
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node_evonode: Node = Node(self.nodes[0], True)
        node_masternode: Node = Node(self.nodes[1], False)
        node_simple: TestNode = self.nodes[2]

        # Initial setup for masternodes
        node_evonode.generate_collateral(self)
        node_masternode.generate_collateral(self)

        node_masternode.register_mn(self, True, "127.0.0.1:9998")
        node_evonode.register_mn(self, True, "127.0.0.1:9997", "19998", "29998")

        node_masternode.update_mn(self, "127.0.0.1:9998")
        node_evonode.update_mn(self, "127.0.0.1:9997", "19998", "29998")

        node_masternode.destroy_mn(self)
        node_evonode.destroy_mn(self)

        node_masternode.register_mn(self, True, "127.0.0.1:9998")
        node_evonode.register_mn(self, True, "127.0.0.1:9997", "19998", "29998")

if __name__ == "__main__":
    NetInfoTest().main()

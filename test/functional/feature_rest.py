#!/usr/bin/env python3
# Copyright (c) 2026 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test REST server command line options, argument validation, and API surface."""

from decimal import Decimal
from openapi_schema_validator import OAS31Validator
import http.client
import json
import os
import re

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    rest_port,
)
from test_framework.wallet import MiniWallet

# See rest::Options in rest/server.h
DEFAULT_ADDR = '127.0.0.1'
DEFAULT_PORT = 19897  # regtest
DEFAULT_THREADS = 1
DEFAULT_MAX_CONNECTIONS = 100
DEFAULT_IDLE_TIMEOUT = 60
DEFAULT_REUSE_PORT = 0

COMMON_ARGS = ['-rest', '-debug=rest']


def starting_server_msg(addr=DEFAULT_ADDR, port=DEFAULT_PORT, threads=DEFAULT_THREADS,
                        max_conn=DEFAULT_MAX_CONNECTIONS, idle_timeout=DEFAULT_IDLE_TIMEOUT,
                        reuseport=DEFAULT_REUSE_PORT):
    return (f'Starting server on {addr}:{port} with {threads} thread(s), '
            f'max_conn={max_conn}, idle_timeout={idle_timeout}, reuseport={reuseport}')


class APIValidator:
    FORMATS = ['json', 'hex', 'bin']

    def __init__(self, spec_path, host, port):
        with open(spec_path, 'r', encoding="utf8") as f:
            self.spec = json.load(f)
        self.host = host
        self.port = port

    def request(self, uri, status=200):
        """GET a URI and return raw response bytes, asserting the status code."""
        conn = http.client.HTTPConnection(self.host, self.port)
        conn.request('GET', uri)
        resp = conn.getresponse()
        body = resp.read()
        assert_equal(resp.status, status)
        return body

    def request_json(self, uri, status=200):
        """GET a .json URI and return parsed JSON."""
        return json.loads(self.request(uri, status).decode('utf-8'), parse_float=Decimal)

    def _resolve_refs(self, schema):
        """Recursively resolve $ref pointers against the loaded spec."""
        if isinstance(schema, dict):
            if '$ref' in schema:
                parts = schema['$ref'].lstrip('#/').split('/')
                resolved = self.spec
                for p in parts:
                    resolved = resolved[p]
                return self._resolve_refs(resolved)
            return {k: self._resolve_refs(v) for k, v in schema.items()}
        if isinstance(schema, list):
            return [self._resolve_refs(item) for item in schema]
        return schema

    def assert_path(self, path_template):
        """Assert that a path template exists in the spec."""
        assert path_template in self.spec['paths'], \
            f"Path {path_template} missing from spec"

    def assert_status(self, path_template, status_code, method='get'):
        """Assert that a status code is documented for a path+method."""
        responses = self.spec['paths'][path_template][method]['responses']
        assert str(status_code) in responses, \
            f"Status {status_code} not documented for {method.upper()} {path_template}"

    def validate_response(self, path_template, obj, status_code=200, method='get'):
        """Validate a JSON response body against the spec's response schema."""
        resp_obj = self.spec['paths'][path_template][method]['responses'][str(status_code)]
        schema = resp_obj['content']['application/json']['schema']
        resolved = self._resolve_refs(schema)
        validator = OAS31Validator(resolved)
        errors = list(validator.iter_errors(obj))
        assert not errors, \
            f"Schema validation failed for {method.upper()} {path_template}:\n" + \
            "\n".join(f"  {e.json_path}: {e.message}" for e in errors)

    def check_endpoint(self, path_pattern, uri, *, json_checks=None):
        """Validate an endpoint across all three formats (.json, .hex, .bin)."""
        # Split query string so the format suffix is inserted before it
        if '?' in uri:
            base, qs = uri.split('?', 1)
            qs = '?' + qs
        else:
            base, qs = uri, ''

        for fmt in self.FORMATS:
            template = f'{path_pattern}.{fmt}'
            self.assert_path(template)
            body = self.request(f'{base}.{fmt}{qs}')
            self.assert_status(template, 200)
            if fmt == 'json':
                obj = json.loads(body.decode('utf-8'), parse_float=Decimal)
                self.validate_response(template, obj)
                if json_checks:
                    json_checks(obj)

    def check_error(self, uri, path_pattern, status):
        """Request a URI expecting an error, and verify the status is in the spec."""
        self.request(uri, status=status)
        self.assert_status(path_pattern, status)

    def check_route_completeness(self, server_routes):
        """Verify spec paths and server routes cover the same set of base paths."""
        spec_routes = set()
        for path in self.spec['paths']:
            spec_routes.add(re.sub(r'\.(json|hex|bin)$', '', path))

        missing = server_routes - spec_routes
        extra = spec_routes - server_routes
        assert not missing, f"Routes in server but not in spec: {missing}"
        assert not extra, f"Routes in spec but not in server: {extra}"


class RESTFeatureTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.rest_port = rest_port(0)

    def skip_test_if_missing_module(self):
        self.skip_if_no_drogon()

    def run_test(self):
        self.log.info("Test invalid command line options")
        self.test_invalid_port()
        self.test_invalid_bind()
        self.test_invalid_idle_timeout()
        self.test_invalid_max_connections()
        self.test_invalid_threads()

        self.log.info("Test command line behavior")
        self.test_clamping_warnings()
        self.test_defaults()

        self.log.info("Test REST API surface against OpenAPI spec")
        self.test_api_surface()

    def test_invalid_port(self):
        self.stop_node(0)
        self.nodes[0].assert_start_raises_init_error(
            expected_msg='Error: Invalid -restport value: 0 (must be between 1 and 65535)',
            extra_args=['-rest', '-restport=0'],
        )
        self.nodes[0].assert_start_raises_init_error(
            expected_msg='Error: Invalid -restport value: -1 (must be between 1 and 65535)',
            extra_args=['-rest', '-restport=-1'],
        )
        self.nodes[0].assert_start_raises_init_error(
            expected_msg='Error: Invalid -restport value: 65536 (must be between 1 and 65535)',
            extra_args=['-rest', '-restport=65536'],
        )

    def test_invalid_bind(self):
        self.nodes[0].assert_start_raises_init_error(
            expected_msg="Error: Cannot resolve -restbind address: 'not_a_valid_addr'",
            extra_args=['-rest', '-restbind=not_a_valid_addr'],
        )

    def test_invalid_idle_timeout(self):
        warning_str = 'Warning: -restidletimeout below 5 seconds or above 3600 seconds, clamped to %d seconds'
        with self.nodes[0].assert_debug_log(expected_msgs=[
            warning_str % 5,
            starting_server_msg(idle_timeout=5),
        ]):
            self.start_node(0, extra_args=COMMON_ARGS + ['-restidletimeout=1'])
        self.stop_node(0, expected_stderr=warning_str % 5)
        with self.nodes[0].assert_debug_log(expected_msgs=[
            warning_str % 3600,
            starting_server_msg(idle_timeout=3600),
        ]):
            self.start_node(0, extra_args=COMMON_ARGS + ['-restidletimeout=7200'])
        self.stop_node(0, expected_stderr=warning_str % 3600)

    def test_invalid_max_connections(self):
        warning_str = 'Warning: -restmaxconnections below 1 connections or above 65535 connections, clamped to 1 connections'
        with self.nodes[0].assert_debug_log(expected_msgs=[
            warning_str,
            starting_server_msg(max_conn=1),
        ]):
            self.start_node(0, extra_args=COMMON_ARGS + ['-restmaxconnections=0'])
        self.stop_node(0, expected_stderr=warning_str)
        with self.nodes[0].assert_debug_log(expected_msgs=[
            warning_str,
            starting_server_msg(max_conn=1),
        ]):
            self.start_node(0, extra_args=COMMON_ARGS + ['-restmaxconnections=-5'])
        self.stop_node(0, expected_stderr=warning_str)

    def test_invalid_threads(self):
        num_cores = os.cpu_count()
        warning_str = 'Warning: -restthreads below 1 thread or above %d threads, clamped to %d threads.'
        warning_low = warning_str % (num_cores, 1)
        warning_high = warning_str % (num_cores, num_cores)
        with self.nodes[0].assert_debug_log(expected_msgs=[
            warning_low,
            starting_server_msg(threads=1),
        ]):
            self.start_node(0, extra_args=COMMON_ARGS + ['-restthreads=0'])
        self.stop_node(0, expected_stderr=warning_low)
        with self.nodes[0].assert_debug_log(expected_msgs=[
            warning_low,
            starting_server_msg(threads=1),
        ]):
            self.start_node(0, extra_args=COMMON_ARGS + ['-restthreads=-1'])
        self.stop_node(0, expected_stderr=warning_low)
        with self.nodes[0].assert_debug_log(expected_msgs=[
            warning_high,
            starting_server_msg(threads=num_cores),
        ]):
            self.start_node(0, extra_args=COMMON_ARGS + ['-restthreads=99999'])
        self.stop_node(0, expected_stderr=warning_high)

    def test_clamping_warnings(self):
        with self.nodes[0].assert_debug_log(
            expected_msgs=[starting_server_msg(threads=1, max_conn=50, idle_timeout=30)],
            unexpected_msgs=[
                'clamped to',
            ],
        ):
            self.start_node(0, extra_args=COMMON_ARGS + ['-restidletimeout=30', '-restmaxconnections=50', '-restthreads=1'])
        self.stop_node(0)

    def test_defaults(self):
        with self.nodes[0].assert_debug_log(expected_msgs=[
            starting_server_msg(),
        ]):
            self.start_node(0, extra_args=COMMON_ARGS)
        self.stop_node(0)

    def test_api_surface(self):
        """Validate every endpoint in openapi.json is reachable and returns documented status codes and response shape."""
        spec_path = os.path.join(os.path.dirname(__file__), '..', '..', 'doc', 'openapi.json')
        v = APIValidator(spec_path, DEFAULT_ADDR, self.rest_port)

        # Start the node with REST enabled on a known port
        self.start_node(0, extra_args=COMMON_ARGS + [
            f'-restport={self.rest_port}',
            '-blockfilterindex=1',
        ])

        self.wallet = MiniWallet(self.nodes[0])
        self.wallet.rescan_utxos()

        # Mine a block so we have chain data to query
        blockhashes = self.generate(self.wallet, 1)
        bb_hash = blockhashes[0]
        block_json = self.nodes[0].getblock(bb_hash)
        coinbase_txid = block_json['tx'][0]
        height = block_json['height']

        # Wait for block filter index to sync
        self.wait_until(lambda: self.nodes[0].getindexinfo().get(
            'basic block filter index', {}).get('synced', False))

        self.log.info("Validate spec paths resolve to documented status codes")

        self.log.info("  /rest/tx")
        v.check_endpoint('/rest/tx/{hash}', f'/rest/tx/{coinbase_txid}',
                         json_checks=lambda obj: assert_equal(obj['txid'], coinbase_txid))

        self.log.info("  /rest/block")
        v.check_endpoint('/rest/block/{hash}', f'/rest/block/{bb_hash}',
                         json_checks=lambda obj: assert_equal(obj['hash'], bb_hash))

        self.log.info("  /rest/block/notxdetails")

        def check_block_notx(obj):
            assert_equal(obj['hash'], bb_hash)
            assert isinstance(obj['tx'][0], str)
        v.check_endpoint('/rest/block/notxdetails/{hash}',
                         f'/rest/block/notxdetails/{bb_hash}',
                         json_checks=check_block_notx)

        self.log.info("  /rest/headers")

        def check_headers(obj):
            assert isinstance(obj, list)
            assert_equal(len(obj), 1)
            assert_equal(obj[0]['hash'], bb_hash)
        v.check_endpoint('/rest/headers/{hash}', f'/rest/headers/{bb_hash}?count=1',
                         json_checks=check_headers)

        # Deprecated path-based count
        v.assert_path('/rest/headers/{count}/{hash}.json')
        assert_equal(
            v.request_json(f'/rest/headers/1/{bb_hash}.json'),
            v.request_json(f'/rest/headers/{bb_hash}.json?count=1'),
        )

        self.log.info("  /rest/blockfilter")
        v.check_endpoint('/rest/blockfilter/{filtertype}/{hash}',
                         f'/rest/blockfilter/basic/{bb_hash}',
                         json_checks=lambda obj: obj['filter'])

        self.log.info("  /rest/blockfilterheaders")

        def check_filterheaders(obj):
            assert isinstance(obj, list)
            assert_equal(len(obj), 1)
        v.check_endpoint('/rest/blockfilterheaders/{filtertype}/{hash}',
                         f'/rest/blockfilterheaders/basic/{bb_hash}?count=1',
                         json_checks=check_filterheaders)
        v.assert_path('/rest/blockfilterheaders/{filtertype}/{count}/{hash}.json')

        self.log.info("  /rest/chaininfo")
        v.assert_path('/rest/chaininfo.json')
        v.assert_status('/rest/chaininfo.json', 200)
        v.assert_status('/rest/chaininfo.json', 404)
        chain_obj = v.request_json('/rest/chaininfo.json')
        v.validate_response('/rest/chaininfo.json', chain_obj)
        rpc_info = self.nodes[0].getblockchaininfo()
        assert_equal(chain_obj['bestblockhash'], rpc_info['bestblockhash'])
        assert_equal(chain_obj['chain'], rpc_info['chain'])
        assert_equal(chain_obj['blocks'], rpc_info['blocks'])

        self.log.info("  /rest/mempool")
        for sub in ['info', 'contents']:
            path = f'/rest/mempool/{sub}.json'
            v.assert_path(path)
            v.assert_status(path, 200)
            obj = v.request_json(f'/rest/mempool/{sub}.json')
            v.validate_response(path, obj)

        self.log.info("  /rest/getutxos")
        v.check_endpoint('/rest/getutxos/{outpoints}',
                         f'/rest/getutxos/{coinbase_txid}-0')

        self.log.info("  /rest/blockhashbyheight")
        v.check_endpoint('/rest/blockhashbyheight/{height}',
                         f'/rest/blockhashbyheight/{height}',
                         json_checks=lambda obj: obj['blockhash'])

        self.log.info("  Error codes")
        invalid_hash = 'not_a_hash'
        unknown_hash = '0' * 64

        # 400 Bad Request - invalid hash
        v.check_error(f'/rest/tx/{invalid_hash}.json', '/rest/tx/{hash}.json', 400)
        v.check_error(f'/rest/block/{invalid_hash}.json', '/rest/block/{hash}.json', 400)
        v.check_error(f'/rest/headers/{invalid_hash}.json?count=1', '/rest/headers/{hash}.json', 400)
        v.check_error(f'/rest/blockhashbyheight/{invalid_hash}.json', '/rest/blockhashbyheight/{height}.json', 400)

        # 400 Bad Request - invalid header count
        v.request(f'/rest/headers/{bb_hash}.json?count=0', status=400)
        v.request(f'/rest/headers/{bb_hash}.json?count=2001', status=400)

        # 400 Bad Request - unknown filtertype
        v.check_error(f'/rest/blockfilter/unknown/{bb_hash}.json',
                      '/rest/blockfilter/{filtertype}/{hash}.json', 400)

        # 404 Not Found - unknown hash
        v.check_error(f'/rest/tx/{unknown_hash}.json', '/rest/tx/{hash}.json', 404)
        v.check_error(f'/rest/block/{unknown_hash}.json', '/rest/block/{hash}.json', 404)

        # 404 Not Found - height out of range
        v.check_error('/rest/blockhashbyheight/999999.json',
                      '/rest/blockhashbyheight/{height}.json', 404)

        # 406 Not Acceptable - chaininfo with non-json format
        v.request('/rest/chaininfo.hex', status=406)

        # 400 Bad Request - invalid mempool sub-resource
        v.request('/rest/mempool/invalid.json', status=400)

        # 400 Bad Request - getutxos with empty request
        v.request('/rest/getutxos.json', status=400)

        self.log.info("Test deprecation headers on suffixed endpoints")

        def get_response(uri):
            conn = http.client.HTTPConnection(DEFAULT_ADDR, self.rest_port)
            conn.request('GET', uri)
            return conn.getresponse()

        # Suffixed paths carry deprecation headers pointing to the suffix-free URI
        for suffix in ['.json', '.hex', '.bin']:
            resp = get_response(f'/rest/block/{bb_hash}{suffix}')
            assert_equal(resp.status, 200)
            assert_equal(resp.getheader('Deprecation'), 'true')
            link = resp.getheader('Link')
            assert f'/rest/block/{bb_hash}' in link, f"Link should contain suffix-free path, got: {link}"
            assert 'successor-version' in link
            resp.read()

        # Suffix-free path should NOT carry deprecation headers
        resp = get_response(f'/rest/block/{bb_hash}')
        assert_equal(resp.status, 200)
        assert resp.getheader('Deprecation') is None, "Suffix-free path should not have Deprecation header"
        resp.read()

        # Legacy count-in-path deprecation takes precedence over suffix deprecation
        resp = get_response(f'/rest/headers/1/{bb_hash}.json')
        assert_equal(resp.status, 200)
        assert_equal(resp.getheader('Deprecation'), 'true')
        link = resp.getheader('Link')
        assert 'count' in link, f"Count-in-path deprecation Link should mention count, got: {link}"
        resp.read()

        self.log.info("Test content negotiation via Accept header")

        def accept_request(uri, accept=None, status=200):
            """Send a request without format suffix, using an Accept header."""
            conn = http.client.HTTPConnection(DEFAULT_ADDR, self.rest_port)
            headers = {"Accept": accept} if accept else {}
            conn.request("GET", uri, headers=headers)
            resp = conn.getresponse()
            body = resp.read()
            assert_equal(resp.status, status)
            return body

        # application/json (matches .json suffix)
        json_accept = json.loads(accept_request(f'/rest/block/{bb_hash}', 'application/json'), parse_float=Decimal)
        json_suffix = v.request_json(f'/rest/block/{bb_hash}.json')
        assert_equal(json_accept, json_suffix)

        # application/octet-stream (matches .bin suffix)
        bin_accept = accept_request(f'/rest/block/{bb_hash}', 'application/octet-stream')
        bin_suffix = v.request(f'/rest/block/{bb_hash}.bin')
        assert_equal(bin_accept, bin_suffix)

        # text/plain (matches .hex suffix)
        hex_accept = accept_request(f'/rest/block/{bb_hash}', 'text/plain')
        hex_suffix = v.request(f'/rest/block/{bb_hash}.hex')
        assert_equal(hex_accept, hex_suffix)

        # empty (defaults to JSON)
        json_default = json.loads(accept_request(f'/rest/block/{bb_hash}'), parse_float=Decimal)
        assert_equal(json_default, json_suffix)

        # */* (defaults to JSON)
        json_wildcard = json.loads(accept_request(f'/rest/block/{bb_hash}', '*/*'), parse_float=Decimal)
        assert_equal(json_wildcard, json_suffix)

        # unsupported type (e.g. text/html), 406 Not Acceptable
        accept_request(f'/rest/block/{bb_hash}', 'text/html', status=406)

        # Suffix takes precedence over Accept header
        hex_override = v.request(f'/rest/block/{bb_hash}.hex')
        assert_equal(hex_override, hex_suffix)

        # Works across different endpoint types
        chain_accept = json.loads(accept_request('/rest/chaininfo', 'application/json'), parse_float=Decimal)
        assert 'chain' in chain_accept
        height_accept = json.loads(accept_request(f'/rest/blockhashbyheight/{height}', 'application/json'), parse_float=Decimal)
        assert 'blockhash' in height_accept

        self.log.info("Validate spec completeness against registered server routes")
        # Known routes from uri_prefixes[] in server.cpp
        v.check_route_completeness({
            '/rest/tx/{hash}',
            '/rest/block/{hash}',
            '/rest/block/notxdetails/{hash}',
            '/rest/headers/{hash}',
            '/rest/headers/{count}/{hash}',
            '/rest/blockfilter/{filtertype}/{hash}',
            '/rest/blockfilterheaders/{filtertype}/{hash}',
            '/rest/blockfilterheaders/{filtertype}/{count}/{hash}',
            '/rest/chaininfo',
            '/rest/mempool/info',
            '/rest/mempool/contents',
            '/rest/getutxos/{outpoints}',
            '/rest/blockhashbyheight/{height}',
        })

        self.stop_node(0)


if __name__ == '__main__':
    RESTFeatureTest().main()

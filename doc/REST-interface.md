Unauthenticated REST Interface
==============================

The REST API can be enabled with the `-rest` option.

The interface runs on its own dedicated port, separate from the JSON-RPC interface.  Default port 9997 for mainnet, port 19997 for testnet, port 19897 for regtest and port 19797 for devnet. The port can be overridden with `-restport=<port>`.

Configuration
-------------

| Option                    | Default     | Description                                                  |
|---------------------------|-------------|--------------------------------------------------------------|
| `-rest`                   | `0`         | Enable the REST server.                                      |
| `-restbind=<addr>`        | `127.0.0.1` | IP address to bind to.                                       |
| `-restport=<port>`        | per-network | Port to listen on (1–65535).                                 |
| `-restthreads=<n>`        | `1`         | Number of I/O threads (1 to number of CPU cores).            |
| `-restmaxconnections=<n>` | `100`       | Maximum concurrent connections (1–65535).                    |
| `-restidletimeout=<n>`    | `60`        | Seconds before an idle connection is closed (5–3600).        |
| `-restreuseport`          | `false`     | Allow multiple sockets to bind to the same port (Linux only).|

REST Interface consistency guarantees
-------------------------------------

The [same guarantees as for the RPC Interface](/doc/JSON-RPC-interface.md#rpc-consistency-guarantees)
apply.

OpenAPI Specification
---------------------

A machine-readable [OpenAPI 3.1 specification](openapi.json) of the REST
interface is available.

Content negotiation
-------------------

The response format is selected through standard HTTP content negotiation
using the `Accept` header:

| Accept value                 | Format   |
|------------------------------|----------|
| `*/*`                        | JSON     |
| `application/json`           | JSON     |
| `application/octet-stream`   | Binary   |
| `text/plain`                 | Hex      |

When no `Accept` header is sent the server defaults to JSON.

> [!WARNING]
> Appending a format suffix (`.json`, `.hex`, `.bin`) to the URI still is **deprecated**.  Suffixed requests receive [RFC 8594](https://www.rfc-editor.org/rfc/rfc8594) `Deprecation` and `Link` headers pointing to the header-negotiated equivalent.

Supported API
-------------

#### Transactions
`GET /rest/tx/<TX-HASH>`

Given a transaction hash: returns a transaction in binary, hex-encoded binary, or JSON formats.
Responds with 404 if the transaction doesn't exist.

By default, this endpoint will only search the mempool.
To query for a confirmed transaction, enable the transaction index via "txindex=1" command line / configuration option.

#### Blocks
- `GET /rest/block/<BLOCK-HASH>`
- `GET /rest/block/notxdetails/<BLOCK-HASH>`

Given a block hash: returns a block, in binary, hex-encoded binary or JSON formats.
Responds with 404 if the block doesn't exist.

The HTTP request and response are both handled entirely in-memory.

With the /notxdetails/ option JSON response will only contain the transaction hash instead of the complete transaction details. The option only affects the JSON response.

#### Blockheaders
`GET /rest/headers/<BLOCK-HASH>?count=<COUNT=5>`

Given a block hash: returns <COUNT> amount of blockheaders in upward direction.
Returns empty if the block doesn't exist or it isn't in the active chain.

*Deprecated (but not removed) since 23.0:*
`GET /rest/headers/<COUNT>/<BLOCK-HASH>`

#### Blockfilter Headers
`GET /rest/blockfilterheaders/<FILTERTYPE>/<BLOCK-HASH>?count=<COUNT=5>`

Given a block hash: returns <COUNT> amount of blockfilter headers in upward
direction for the filter type <FILTERTYPE>.
Returns empty if the block doesn't exist or it isn't in the active chain.

*Deprecated (but not removed) since 23.0:*
`GET /rest/blockfilterheaders/<FILTERTYPE>/<COUNT>/<BLOCK-HASH>`

#### Blockfilters
`GET /rest/blockfilter/<FILTERTYPE>/<BLOCK-HASH>`

Given a block hash: returns the block filter of the given block of type
<FILTERTYPE>.
Responds with 404 if the block doesn't exist.

#### Blockhash by height
`GET /rest/blockhashbyheight/<HEIGHT>`

Given a height: returns hash of block in best-block-chain at height provided.
Responds with 404 if block not found.

#### Quorum List
`GET /rest/quorum/list`

Returns the extended list of on-chain quorums at the chain tip, or at the height specified by the optional `height` query parameter.
Only supports JSON as output format.

#### ProTx Diff
`GET /rest/protx/diff/<BASE-HEIGHT>/<BLOCK-HEIGHT>`

Returns the deterministic masternode list diff between two block heights, including proof data.
Only supports JSON as output format.
The optional query parameter `extended=true` includes additional fields.

#### ChainLock
`GET /rest/chainlock`

Returns the best known ChainLock.
Only supports JSON as output format.
Responds with 404 if no ChainLock is known yet.

#### Chaininfos
`GET /rest/chaininfo`

Returns various state info regarding block chain processing.
Only supports JSON as output format.
Refer to the `getblockchaininfo` RPC help for details.

#### Query UTXO set
- `GET /rest/getutxos/<TXID>-<N>/<TXID>-<N>/.../<TXID>-<N>`
- `GET /rest/getutxos/checkmempool/<TXID>-<N>/<TXID>-<N>/.../<TXID>-<N>`

The getutxos endpoint allows querying the UTXO set, given a set of outpoints.
With the `/checkmempool/` option, the mempool is also taken into account.
See [BIP64](https://github.com/bitcoin/bips/blob/master/bip-0064.mediawiki) for
input and output serialization (relevant for binary and hex output formats).

Example:
```
$ curl -H "Accept: application/json" localhost:19997/rest/getutxos/checkmempool/b2cdfd7b89def827ff8af7cd9bff7627ff72e5e8b0f71210f92ea7a4000c5d75-0 2>/dev/null | json_pp
{
   "chainHeight" : 325347,
   "chaintipHash" : "00000000fb01a7f3745a717f8caebee056c484e6e0bfe4a9591c235bb70506fb",
   "bitmap": "1",
   "utxos" : [
      {
         "height" : 2147483647,
         "value" : 8.8687,
         "scriptPubKey" : {
            "asm" : "OP_DUP OP_HASH160 1c7cebb529b86a04c683dfa87be49de35bcf589e OP_EQUALVERIFY OP_CHECKSIG",
            "desc" : "addr(mi7as51dvLJsizWnTMurtRmrP8hG2m1XvD)#gj9tznmy"
            "hex" : "76a9141c7cebb529b86a04c683dfa87be49de35bcf589e88ac",
            "type" : "pubkeyhash",
            "address" : "mi7as51dvLJsizWnTMurtRmrP8hG2m1XvD"
         }
      }
   ]
}
```

#### Governance Proposals

`GET /rest/governance/proposals`

Returns all valid governance proposals, sorted newest-first.
Only supports JSON as output format.

`GET /rest/governance/proposal/<PROPOSAL-HASH>`

Given a proposal hash: returns a single governance proposal.
Only supports JSON as output format.
Responds with 404 if the proposal doesn't exist or is not a valid proposal.

#### Memory pool
`GET /rest/mempool/info`

Returns various information about the transaction mempool.
Only supports JSON as output format.
Refer to the `getmempoolinfo` RPC help for details.

`GET /rest/mempool/contents`

Returns the transactions in the mempool.
Only supports JSON as output format.
Refer to the `getrawmempool` RPC help for details.

Risks
-------------
Running a web browser on the same node with a REST enabled dashd can be a risk. Accessing prepared XSS websites could read out tx/block data of your node by placing links like `<script src="http://127.0.0.1:19997/rest/tx/1234567890.json">` which might break the nodes privacy.

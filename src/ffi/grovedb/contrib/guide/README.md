<p align="center">
  <img src="assets/grovedb-logo.svg" alt="GroveDB" width="400">
</p>

# GroveDB for C++

GroveDB is a hierarchical authenticated database built on Merkle trees. It enables cryptographic proofs for complex queries — range queries, secondary index lookups, and aggregate computations — all verifiable against a single 32-byte root hash. This library provides idiomatic C++20 bindings over the Rust engine via FFI.

## Who Is This For?

C++ developers building blockchain applications that need **provable state queries**. GroveDB is a hierarchical authenticated state store where secondary indexes are first-class citizens with cryptographic proofs.

## Documentation

| Chapter | Description |
|---------|-------------|
| [What Is GroveDB?](01-what-is-grovedb.md) | The problem GroveDB solves — from flat Merkle trees to a grove of authenticated trees |
| [Architecture](02-architecture.md) | Three-layer stack, tree-of-trees hierarchy, paths, keys, and the root hash |
| [Getting Started](03-getting-started.md) | Build, link, and open your first database |
| [Elements and Trees](04-elements-and-trees.md) | Element types, subtree hierarchies, and paths |
| [Basic Operations](05-basic-operations.md) | Put, Get, Delete, and conditional variants |
| [Queries](06-queries.md) | PathQuery, range predicates, subqueries, and multi-queries |
| [Proofs](07-proofs.md) | Generate, verify, absence proofs, and chained verification |
| [Transactions](08-transactions.md) | Begin, Commit, Rollback, and RAII auto-rollback |
| [Batch Operations](09-batch-operations.md) | Atomic multi-operation batches |
| [Advanced Topics](10-advanced-topics.md) | Checkpoints, auxiliary storage, cost tracking, integrity, sum trees |
| [Error Handling](11-error-handling.md) | Result type, imperative and chained styles, error codes |
| [Glossary](glossary.md) | Terminology reference |

## Quick Links

- **API Reference**: Generated Doxygen documentation from `include/grovedb/` headers
- **Example Programs**: `contrib/examples/` — 19 self-contained programs demonstrating every feature
- **Source Repository**: [github.com/dashpay/grovedb](https://github.com/dashpay/grovedb)

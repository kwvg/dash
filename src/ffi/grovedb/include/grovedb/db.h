// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GROVEDB_DB_H
#define GROVEDB_DB_H

#include <grovedb/batch.h>
#include <grovedb/cost.h>
#include <grovedb/element.h>
#include <grovedb/proof.h>
#include <grovedb/query.h>
#include <grovedb/status.h>
#include <grovedb/transaction.h>
#include <grovedb/types.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace grovedb {
class Db
{
public:
    Db();
    ~Db();

    Db(const Db&) = delete;
    Db& operator=(const Db&) = delete;
    Db(Db&&);
    Db& operator=(Db&&);

    /**
     * Open or create a GroveDB instance at the given filesystem path.
     *
     * @param[in]  path  Directory where the database files are stored.
     * @param[out] db    Receives the opened database handle on success.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    static Status Open(const std::string& path, Db& db);

    /** Flush the in-memory write buffer to persistent storage. */
    Status Flush();

    /** Delete all GroveDB key-value pairs from the underlying storage. */
    Status Destroy();

    /**
     * Compute the 32-byte Merkle root hash together with operation costs.
     *
     * @param[out] hash  Receives the 32-byte root hash.
     * @param[out] cost  Receives the operation resource counters.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status GetRootHash(Hash& hash, OperationCost& cost);

    /**
     * Verify cryptographic integrity of the entire tree.
     *
     * @param[out] result  Set to true when no issues are found.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status VerifyIntegrity(bool& result);

    /**
     * Begin a new transaction.
     *
     * @param[out] txn  Receives the transaction handle on success.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status BeginTransaction(Transaction& txn);

    /**
     * Commit an active transaction to persistent storage.
     *
     * The transaction is consumed — no further operations are possible
     * on the handle after a successful or failed commit.
     *
     * @param[in,out] txn   The transaction to commit.
     * @param[out]    cost  Receives the operation resource counters.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status Commit(Transaction& txn, OperationCost& cost);

    /**
     * Explicitly roll back an active transaction.
     *
     * @param[in,out] txn  The transaction to roll back.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status Rollback(Transaction& txn);

    // -- Get operations ---------------------------------------------------

    /**
     * Get an element by path and key, following any references.
     *
     * @param[in]  path     Path to the subtree.
     * @param[in]  key      Key within the subtree.
     * @param[out] element  Receives the element on success.
     * @param[out] cost     Receives the operation resource counters.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status Get(const Path& path, const Bytes& key, Element& element, OperationCost& cost);
    Status Get(const Path& path, const Bytes& key, const Transaction& txn, Element& element, OperationCost& cost);

    /**
     * Get an element by path and key without following references.
     *
     * @param[in]  path     Path to the subtree.
     * @param[in]  key      Key within the subtree.
     * @param[out] element  Receives the element on success.
     * @param[out] cost     Receives the operation resource counters.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status GetDirect(const Path& path, const Bytes& key, Element& element, OperationCost& cost);
    Status GetDirect(const Path& path, const Bytes& key, const Transaction& txn, Element& element, OperationCost& cost);

    /**
     * Get an element optionally — returns std::nullopt instead of an error
     * when the key is not found.
     *
     * @param[in]  path     Path to the subtree.
     * @param[in]  key      Key within the subtree.
     * @param[out] element  Receives the element, or std::nullopt if not found.
     * @param[out] cost     Receives the operation resource counters.
     * @return Status::Ok() on success (even when not found); an error Status
     *         only for unexpected failures.
     */
    Status GetOptional(const Path& path, const Bytes& key, std::optional<Element>& element, OperationCost& cost);
    Status GetOptional(const Path& path, const Bytes& key, const Transaction& txn, std::optional<Element>& element, OperationCost& cost);

    /**
     * Check whether a key exists at the given path (no reference following).
     *
     * @param[in]  path    Path to the subtree.
     * @param[in]  key     Key to check.
     * @param[out] result  Set to true if the key exists.
     * @param[out] cost    Receives the operation resource counters.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status KeyExists(const Path& path, const Bytes& key, bool& result, OperationCost& cost);
    Status KeyExists(const Path& path, const Bytes& key, const Transaction& txn, bool& result, OperationCost& cost);

    /**
     * Check whether a subtree path is valid (all parent subtrees exist).
     *
     * @param[in]  path    Path to validate.
     * @param[out] result  Set to true if the path is valid.
     * @param[out] cost    Receives the operation resource counters.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status SubtreeExists(const Path& path, bool& result, OperationCost& cost);
    Status SubtreeExists(const Path& path, const Transaction& txn, bool& result, OperationCost& cost);

    /**
     * Check whether a subtree at the given path is empty.
     *
     * @param[in]  path   The path to the subtree.
     * @param[out] empty  Set to true if the subtree is empty.
     * @param[out] cost   Receives the operation resource counters.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status IsEmptyTree(const Path& path, bool& empty, OperationCost& cost);
    Status IsEmptyTree(const Path& path, const Transaction& txn, bool& empty, OperationCost& cost);

    // -- Put operations ---------------------------------------------------

    /**
     * Insert or replace an element at the given path and key.
     *
     * @param[in]  path     Path to the subtree.
     * @param[in]  key      Key within the subtree.
     * @param[in]  element  Element to store (serialized via Element factories).
     * @param[out] cost     Receives the operation resource counters.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status Put(const Path& path, const Bytes& key, const Element& element, OperationCost& cost);
    Status Put(const Path& path, const Bytes& key, const Element& element, const Transaction& txn, OperationCost& cost);

    /**
     * Insert an element only if the key does not already exist.
     *
     * @param[in]  path      Path to the subtree.
     * @param[in]  key       Key within the subtree.
     * @param[in]  element   Element to store.
     * @param[out] inserted  Set to true if the element was inserted.
     * @param[out] cost      Receives the operation resource counters.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status PutIfAbsent(const Path& path, const Bytes& key, const Element& element, bool& inserted, OperationCost& cost);
    Status PutIfAbsent(const Path& path, const Bytes& key, const Element& element, const Transaction& txn, bool& inserted, OperationCost& cost);

    /**
     * Insert an element if the key does not exist; return the existing
     * element if it does.
     *
     * @param[in]  path      Path to the subtree.
     * @param[in]  key       Key within the subtree.
     * @param[in]  element   Element to store if absent.
     * @param[out] existing  Receives the existing element, or std::nullopt
     *                       when the new element was inserted.
     * @param[out] cost      Receives the operation resource counters.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status PutIfAbsentAndGet(const Path& path, const Bytes& key, const Element& element, std::optional<Element>& existing, OperationCost& cost);
    Status PutIfAbsentAndGet(const Path& path, const Bytes& key, const Element& element, const Transaction& txn, std::optional<Element>& existing, OperationCost& cost);

    /**
     * Insert an element only if its value differs from the existing one.
     *
     * @param[in]  path      Path to the subtree.
     * @param[in]  key       Key within the subtree.
     * @param[in]  element   Element to store.
     * @param[out] changed   Set to true if a new value was written.
     * @param[out] previous  Receives the previous element if it existed
     *                       and was replaced, or std::nullopt otherwise.
     * @param[out] cost      Receives the operation resource counters.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status PutIfChanged(const Path& path, const Bytes& key, const Element& element, bool& changed, std::optional<Element>& previous, OperationCost& cost);
    Status PutIfChanged(const Path& path, const Bytes& key, const Element& element, const Transaction& txn, bool& changed, std::optional<Element>& previous, OperationCost& cost);

    // -- Delete operations ------------------------------------------------

    /**
     * Delete the element at the given path and key.
     *
     * @param[in]  path  Path to the subtree.
     * @param[in]  key   Key to delete.
     * @param[out] cost  Receives the operation resource counters.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status Delete(const Path& path, const Bytes& key, OperationCost& cost);
    Status Delete(const Path& path, const Bytes& key, const Transaction& txn, OperationCost& cost);

    /**
     * Delete the element only if it is an empty subtree.
     *
     * @param[in]  path     Path to the subtree.
     * @param[in]  key      Key to conditionally delete.
     * @param[out] deleted  Set to true if the element was deleted.
     * @param[out] cost     Receives the operation resource counters.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status DeleteIfEmpty(const Path& path, const Bytes& key, bool& deleted, OperationCost& cost);
    Status DeleteIfEmpty(const Path& path, const Bytes& key, const Transaction& txn, bool& deleted, OperationCost& cost);

    /**
     * Delete the element and recursively remove empty parent subtrees.
     *
     * @param[in]  path           Path to the subtree.
     * @param[in]  key            Key to delete.
     * @param[out] removed_count  Number of levels removed (including the
     *                            target element).
     * @param[out] cost           Receives the operation resource counters.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status PruneEmptyAncestors(const Path& path, const Bytes& key, uint32_t& removed_count, OperationCost& cost);
    Status PruneEmptyAncestors(const Path& path, const Bytes& key, const Transaction& txn, uint32_t& removed_count, OperationCost& cost);

    /**
     * Remove all elements within the subtree at the given path.
     *
     * @param[in]  path    Path to the subtree to clear.
     * @param[out] result  Set to true if the subtree was cleared.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status Clear(const Path& path, bool& result);
    Status Clear(const Path& path, const Transaction& txn, bool& result);

    // -- Query operations -------------------------------------------------

    /**
     * Execute a query and return the raw item values.
     *
     * @param[in]  query    A PathQuery describing what to retrieve.
     * @param[out] values   Receives the result values (raw item bytes).
     * @param[out] skipped  Number of results skipped due to offset.
     * @param[out] cost     Receives the operation resource counters.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status QueryValues(const PathQuery& query, std::vector<Bytes>& values, uint16_t& skipped, OperationCost& cost);
    Status QueryValues(const PathQuery& query, const Transaction& txn, std::vector<Bytes>& values, uint16_t& skipped, OperationCost& cost);

    /**
     * Execute a query returning items, sum values, or count values.
     *
     * Handles SumTree, CountTree, and mixed element queries.
     *
     * @param[in]  query    A PathQuery describing what to retrieve.
     * @param[out] results  Receives the tagged-union result entries.
     * @param[out] skipped  Number of results skipped due to offset.
     * @param[out] cost     Receives the operation resource counters.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status QueryItemsOrSums(const PathQuery& query, std::vector<QueryItemOrSum>& results, uint16_t& skipped, OperationCost& cost);
    Status QueryItemsOrSums(const PathQuery& query, const Transaction& txn, std::vector<QueryItemOrSum>& results, uint16_t& skipped, OperationCost& cost);

    /**
     * Execute a query returning only sum values (i64).
     *
     * Returns an error if any result element is not a sum value.
     *
     * @param[in]  query    A PathQuery describing what to retrieve.
     * @param[out] sums     Receives the i64 sum values.
     * @param[out] skipped  Number of results skipped due to offset.
     * @param[out] cost     Receives the operation resource counters.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status QuerySums(const PathQuery& query, std::vector<int64_t>& sums, uint16_t& skipped, OperationCost& cost);
    Status QuerySums(const PathQuery& query, const Transaction& txn, std::vector<int64_t>& sums, uint16_t& skipped, OperationCost& cost);

    /**
     * Execute a raw query returning full Element structures.
     *
     * Does not follow references.
     *
     * @param[in]  query        A PathQuery describing what to retrieve.
     * @param[in]  result_type  0=Element, 1=KeyElementPair, 2=PathKeyElementTrio.
     * @param[out] elements     Receives the result elements.
     * @param[out] skipped      Number of results skipped due to offset.
     * @param[out] cost         Receives the operation resource counters.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status QueryRaw(const PathQuery& query, uint8_t result_type, std::vector<QueryResultElement>& elements, uint16_t& skipped, OperationCost& cost);
    Status QueryRaw(const PathQuery& query, uint8_t result_type, const Transaction& txn, std::vector<QueryResultElement>& elements, uint16_t& skipped, OperationCost& cost);

    struct RawQuerySpec {
        const Path& path;
        const std::vector<QueryItem>& items;
        uint32_t limit{0};
        uint32_t offset{0};
    };

    /**
     * Execute multiple raw queries atomically with merged results.
     *
     * Each query is specified as a tuple of (path, items, limit, offset) —
     * the same arguments as PathQuery::New().  They are wire-encoded and
     * sent to GroveDB as a batch.
     *
     * @param[in]  queries      The path queries to execute.
     * @param[in]  result_type  0=Element, 1=KeyElementPair, 2=PathKeyElementTrio.
     * @param[out] elements     Receives the merged result elements.
     * @param[out] cost         Receives the operation resource counters.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status QueryManyRaw(const std::vector<RawQuerySpec>& queries, uint8_t result_type, std::vector<QueryResultElement>& elements, OperationCost& cost);

    /**
     * Execute a query returning path-key-element triples with optional elements.
     *
     * Follows references. Missing keys appear with std::nullopt for the element.
     * The query must have a limit set.
     *
     * @param[in]  query    A PathQuery describing what to retrieve.
     * @param[out] results  Receives the path-key-element triples.
     * @param[out] cost     Receives the operation resource counters.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status QueryKeysOptional(const PathQuery& query, std::vector<PathKeyElement>& results, OperationCost& cost);
    Status QueryKeysOptional(const PathQuery& query, const Transaction& txn, std::vector<PathKeyElement>& results, OperationCost& cost);

    /**
     * Execute a raw query returning path-key-element triples with optional
     * elements.  Does not follow references.
     *
     * @param[in]  query    A PathQuery describing what to retrieve.
     * @param[out] results  Receives the path-key-element triples.
     * @param[out] cost     Receives the operation resource counters.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status QueryRawKeysOptional(const PathQuery& query, std::vector<PathKeyElement>& results, OperationCost& cost);
    Status QueryRawKeysOptional(const PathQuery& query, const Transaction& txn, std::vector<PathKeyElement>& results, OperationCost& cost);

    // -- Proof operations -------------------------------------------------

    /**
     * Generate a cryptographic proof for a query.
     *
     * @param[in]  query  The path query to prove.
     * @param[out] proof  Receives the opaque proof bytes.
     * @param[out] cost   Receives the operation resource counters.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status Prove(const PathQuery& query, Bytes& proof, OperationCost& cost);
    Status Prove(const PathQuery& query, const ProveOptions& options,
                 Bytes& proof, OperationCost& cost);

    /**
     * Verify a proof against a query (no database instance needed).
     *
     * @param[in]  proof      Opaque proof bytes from Prove().
     * @param[in]  query      The query the proof was generated for.
     * @param[out] root_hash  Receives the 32-byte Merkle root from the proof.
     * @param[out] entries    Receives the verified path-key-element triples.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    static Status VerifyQuery(
        const Bytes& proof, const PathQuery& query,
        Hash& root_hash, std::vector<ProofResultEntry>& entries);
    static Status VerifyQuery(
        const Bytes& proof, const PathQuery& query,
        const VerifyOptions& options,
        Hash& root_hash, std::vector<ProofResultEntry>& entries);

    /**
     * Verify a subset proof (proof may contain more data than the query
     * requests).
     *
     * @param[in]  proof      Opaque proof bytes.
     * @param[in]  query      The query to verify against.
     * @param[out] root_hash  Receives the 32-byte Merkle root from the proof.
     * @param[out] entries    Receives the verified path-key-element triples.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    static Status VerifySubsetQuery(
        const Bytes& proof, const PathQuery& query,
        Hash& root_hash, std::vector<ProofResultEntry>& entries);

    /**
     * Verify a query and check for absence proofs of non-existing keys.
     *
     * @param[in]  proof      Opaque proof bytes.
     * @param[in]  query      The query to verify against.
     * @param[out] root_hash  Receives the 32-byte Merkle root from the proof.
     * @param[out] entries    Receives the verified path-key-element triples.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    static Status VerifyQueryWithAbsenceProof(
        const Bytes& proof, const PathQuery& query,
        Hash& root_hash, std::vector<ProofResultEntry>& entries);

    /**
     * Verify a subset query with absence proofs.
     *
     * @param[in]  proof      Opaque proof bytes.
     * @param[in]  query      The query to verify against.
     * @param[out] root_hash  Receives the 32-byte Merkle root from the proof.
     * @param[out] entries    Receives the verified path-key-element triples.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    static Status VerifySubsetQueryWithAbsenceProof(
        const Bytes& proof, const PathQuery& query,
        Hash& root_hash, std::vector<ProofResultEntry>& entries);

    /**
     * Verify a proof with chained path queries.
     *
     * Each chained query is applied unconditionally after the first query.
     * The C++ side provides pre-computed queries rather than closures.
     *
     * @param[in]  proof           Opaque proof bytes.
     * @param[in]  first_query     The initial query to verify against.
     * @param[in]  chained_queries Subsequent queries applied in order.
     * @param[out] root_hash       Receives the 32-byte Merkle root from the proof.
     * @param[out] all_results     Receives one result set per query
     *                             (first + chained).
     * @return Status::Ok() on success; an error Status otherwise.
     */
    static Status VerifyChainedQueries(
        const Bytes& proof,
        const PathQuery& first_query,
        const std::vector<const PathQuery*>& chained_queries,
        Hash& root_hash,
        std::vector<std::vector<ProofResultEntry>>& all_results);

    // -- Auxiliary data operations ----------------------------------------

    /**
     * Store auxiliary key-value data outside the Merkle tree structure.
     *
     * Auxiliary data is not part of the tree's cryptographic commitment
     * but shares the same transactional guarantees.
     *
     * @param[in]  key   The auxiliary key.
     * @param[in]  value The value to store.
     * @param[out] cost  Receives the operation resource counters.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status PutAux(const Bytes& key, const Bytes& value, OperationCost& cost);
    Status PutAux(const Bytes& key, const Bytes& value, const Transaction& txn, OperationCost& cost);

    /**
     * Retrieve auxiliary data by key.
     *
     * @param[in]  key    The auxiliary key.
     * @param[out] value  Receives the value, or std::nullopt if not found.
     * @param[out] cost   Receives the operation resource counters.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status GetAux(const Bytes& key, std::optional<Bytes>& value, OperationCost& cost);
    Status GetAux(const Bytes& key, const Transaction& txn, std::optional<Bytes>& value, OperationCost& cost);

    /**
     * Delete auxiliary data by key.
     *
     * @param[in]  key   The auxiliary key to delete.
     * @param[out] cost  Receives the operation resource counters.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status DeleteAux(const Bytes& key, OperationCost& cost);
    Status DeleteAux(const Bytes& key, const Transaction& txn, OperationCost& cost);

    /**
     * Find all subtrees under a given path.
     *
     * Returns the full paths of all subtrees found recursively.
     *
     * @param[in]  path      The path to search under.
     * @param[out] subtrees  Receives the list of subtree paths.
     * @param[out] cost      Receives the operation resource counters.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status FindSubtrees(const Path& path, std::vector<Path>& subtrees, OperationCost& cost);
    Status FindSubtrees(const Path& path, const Transaction& txn, std::vector<Path>& subtrees, OperationCost& cost);

    // -- Batch operations -------------------------------------------------

    /**
     * Atomically apply a batch of operations.
     *
     * @param[in]  ops   Operations to apply (constructed via BatchOperation
     *                    factory methods).
     * @param[out] cost  Receives the operation resource counters.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    Status ApplyBatch(const std::vector<BatchOperation>& ops,
                      OperationCost& cost);
    Status ApplyBatch(const std::vector<BatchOperation>& ops,
                      const BatchApplyOptions& options,
                      OperationCost& cost);
    Status ApplyBatch(const std::vector<BatchOperation>& ops,
                      const Transaction& txn,
                      OperationCost& cost);
    Status ApplyBatch(const std::vector<BatchOperation>& ops,
                      const BatchApplyOptions& options,
                      const Transaction& txn,
                      OperationCost& cost);

private:
    /** Wire-encode batch operations for the FFI boundary. */
    static Bytes EncodeBatchOps(const std::vector<BatchOperation>& ops);

    /** Wire-encode multiple raw query specs for query_many_raw. */
    static Bytes EncodeManyQueries(const std::vector<RawQuerySpec>& queries);

    /** Decode wire-encoded verification result into C++ types. */
    static Status DecodeVerifyResult(
        std::span<const uint8_t> root_hash_bytes,
        std::span<const uint8_t> entries_bytes,
        Hash& root_hash,
        std::vector<ProofResultEntry>& entries);

    /** Decode wire-encoded QueryItemOrSum results. */
    static Status DecodeItemsOrSums(
        std::span<const uint8_t> data,
        std::vector<QueryItemOrSum>& results);

    /** Decode wire-encoded i64 sums. */
    static Status DecodeSums(
        std::span<const uint8_t> data,
        std::vector<int64_t>& sums);

    /** Decode wire-encoded QueryResultElement entries. */
    static Status DecodeQueryResultElements(
        std::span<const uint8_t> data,
        std::vector<QueryResultElement>& elements);

    /** Decode wire-encoded PathKeyOptionalElementTrio entries. */
    static Status DecodePathKeyElements(
        std::span<const uint8_t> data,
        std::vector<PathKeyElement>& results);

    /** Decode wire-encoded find_subtrees result into a vector of paths. */
    static Status DecodePaths(
        std::span<const uint8_t> data,
        std::vector<Path>& paths);

    /** Decode wire-encoded chained verification result sets. */
    static Status DecodeChainedVerifyResult(
        std::span<const uint8_t> root_hash_bytes,
        std::span<const uint8_t> result_sets_bytes,
        Hash& root_hash,
        std::vector<std::vector<ProofResultEntry>>& all_results);

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

/** Return a human-readable build identification string. */
[[nodiscard]] std::string GetWhoami();
} // namespace grovedb

#endif // GROVEDB_DB_H

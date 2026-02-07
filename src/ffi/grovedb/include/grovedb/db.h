// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#ifndef LIBGROVEDB_DB_H
#define LIBGROVEDB_DB_H

#include <grovedb/cost.h>
#include <grovedb/costed.h>
#include <grovedb/element.h>
#include <grovedb/error.h>
#include <grovedb/result.h>
#include <grovedb/transaction.h>
#include <grovedb/types.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace grovedb {
// Forward declarations for query types (defined in grovedb/query.h).
class PathQuery;
class QueryItem;
struct QueryItemOrSum;
struct QueryResultElement;
struct PathKeyElement;
template <typename T>
struct QueryData;

// Forward declarations for batch types (defined in grovedb/batch.h).
class BatchOperation;
struct BatchApplyOptions;

/**
 * Primary handle to a GroveDB database instance.
 *
 * Move-only. Operations return Result<T, Error> for monadic composition.
 */
class Db
{
public:
  Db();
  ~Db();

  Db(const Db&) = delete;
  Db& operator=(const Db&) = delete;
  Db(Db&&);
  Db& operator=(Db&&);

  // -- Lifecycle ------------------------------------------------------------

  /**
   * Open or create a GroveDB instance at the given filesystem path.
   *
   * @param[in] path  Directory where the database files are stored.
   * @return The opened database handle, or an error.
   */
  [[nodiscard]] static Result<Db, Error> Open(const std::string& path);

  /** Flush the in-memory write buffer to persistent storage. */
  [[nodiscard]] Result<void, Error> Flush();

  /** Delete all GroveDB key-value pairs from the underlying storage. */
  [[nodiscard]] Result<void, Error> Destroy();

  /**
   * Compute the 32-byte Merkle root hash together with operation costs.
   *
   * @return The root hash bundled with operation costs, or an error.
   */
  [[nodiscard]] Result<Costed<Hash>, Error> GetRootHash();

  /**
   * Verify cryptographic integrity of the entire tree.
   *
   * @return True when no issues are found, or an error.
   */
  [[nodiscard]] Result<bool, Error> VerifyIntegrity();

  // -- Transactions ---------------------------------------------------------

  /**
   * Begin a new transaction.
   *
   * @return The transaction handle, or an error.
   */
  [[nodiscard]] Result<Transaction, Error> BeginTransaction();

  /**
   * Commit an active transaction to persistent storage.
   *
   * The transaction is consumed — no further operations are possible
   * on the handle after a successful or failed commit.
   *
   * @param[in,out] txn  The transaction to commit.
   * @return Operation costs, or an error.
   */
  [[nodiscard]] Result<OperationCost, Error> Commit(Transaction& txn);

  /**
   * Explicitly roll back an active transaction.
   *
   * @param[in,out] txn  The transaction to roll back.
   */
  [[nodiscard]] Result<void, Error> Rollback(Transaction& txn);

  // -- Get operations -------------------------------------------------------

  /**
   * Get an element by path and key, following any references.
   *
   * @param[in] path  The tree path.
   * @param[in] key   The key within the subtree.
   * @return The element with operation costs, or an error.
   */
  [[nodiscard]] Result<Costed<Element>, Error> Get(const Path& path, const Bytes& key);
  [[nodiscard]] Result<Costed<Element>, Error>
  Get(const Path& path, const Bytes& key, const Transaction& txn);

  /**
   * Get an element by path and key without following references.
   *
   * @param[in] path  The tree path.
   * @param[in] key   The key within the subtree.
   * @return The element with operation costs, or an error.
   */
  [[nodiscard]] Result<Costed<Element>, Error> GetDirect(const Path& path, const Bytes& key);
  [[nodiscard]] Result<Costed<Element>, Error>
  GetDirect(const Path& path, const Bytes& key, const Transaction& txn);

  /**
   * Get an element optionally — returns nullopt instead of error when missing.
   *
   * @param[in] path  The tree path.
   * @param[in] key   The key within the subtree.
   * @return The optional element with operation costs, or an error.
   */
  [[nodiscard]] Result<Costed<std::optional<Element>>, Error>
  GetOptional(const Path& path, const Bytes& key);
  [[nodiscard]] Result<Costed<std::optional<Element>>, Error>
  GetOptional(const Path& path, const Bytes& key, const Transaction& txn);

  /**
   * Check whether a key exists at the given path (no reference following).
   *
   * @param[in] path  The tree path.
   * @param[in] key   The key within the subtree.
   * @return True if the key exists, with operation costs.
   */
  [[nodiscard]] Result<Costed<bool>, Error> KeyExists(const Path& path, const Bytes& key);
  [[nodiscard]] Result<Costed<bool>, Error>
  KeyExists(const Path& path, const Bytes& key, const Transaction& txn);

  /**
   * Check whether all parent subtrees in the path exist.
   *
   * @param[in] path  The tree path to validate.
   * @return True when the path is valid, with operation costs.
   */
  [[nodiscard]] Result<Costed<bool>, Error> SubtreeExists(const Path& path);
  [[nodiscard]] Result<Costed<bool>, Error> SubtreeExists(const Path& path, const Transaction& txn);

  /**
   * Check whether the subtree at the given path is empty.
   *
   * @param[in] path  The tree path.
   * @return True if the subtree is empty, with operation costs.
   */
  [[nodiscard]] Result<Costed<bool>, Error> IsEmptyTree(const Path& path);
  [[nodiscard]] Result<Costed<bool>, Error> IsEmptyTree(const Path& path, const Transaction& txn);

  // -- Put operations -------------------------------------------------------

  /**
   * Insert an element at the given path and key.
   *
   * @param[in] path     The tree path.
   * @param[in] key      The key within the subtree.
   * @param[in] element  The element to insert.
   * @return Operation costs, or an error.
   */
  [[nodiscard]] Result<OperationCost, Error>
  Put(const Path& path, const Bytes& key, const Element& element);
  [[nodiscard]] Result<OperationCost, Error>
  Put(const Path& path, const Bytes& key, const Element& element, const Transaction& txn);

  /**
   * Insert an element only if the key does not already exist.
   *
   * @param[in] path     The tree path.
   * @param[in] key      The key within the subtree.
   * @param[in] element  The element to insert.
   * @return True if inserted (key was absent), with operation costs.
   */
  [[nodiscard]] Result<Costed<bool>, Error>
  PutIfAbsent(const Path& path, const Bytes& key, const Element& element);
  [[nodiscard]] Result<Costed<bool>, Error>
  PutIfAbsent(const Path& path, const Bytes& key, const Element& element, const Transaction& txn);

  /**
   * Insert an element if the key does not exist; return the existing element
   * if it does.
   *
   * @param[in] path     The tree path.
   * @param[in] key      The key within the subtree.
   * @param[in] element  The element to insert.
   * @return Nullopt if inserted, or the existing element; with operation costs.
   */
  [[nodiscard]] Result<Costed<std::optional<Element>>, Error>
  PutIfAbsentAndGet(const Path& path, const Bytes& key, const Element& element);
  [[nodiscard]] Result<Costed<std::optional<Element>>, Error> PutIfAbsentAndGet(
      const Path& path, const Bytes& key, const Element& element, const Transaction& txn
  );

  /** Result of a conditional insert when the value may have changed. */
  struct ChangedValue {
    bool m_changed;
    std::optional<Element> m_previous;
  };

  /**
   * Insert an element only if the value differs from the existing one.
   *
   * @param[in] path     The tree path.
   * @param[in] key      The key within the subtree.
   * @param[in] element  The element to insert.
   * @return {changed, optional previous element} with operation costs.
   */
  [[nodiscard]] Result<Costed<ChangedValue>, Error>
  PutIfChanged(const Path& path, const Bytes& key, const Element& element);
  [[nodiscard]] Result<Costed<ChangedValue>, Error>
  PutIfChanged(const Path& path, const Bytes& key, const Element& element, const Transaction& txn);

  // -- Delete operations ----------------------------------------------------

  /**
   * Delete the element at the given path and key.
   *
   * @param[in] path  The tree path.
   * @param[in] key   The key to delete.
   * @return Operation costs, or an error.
   */
  [[nodiscard]] Result<OperationCost, Error> Delete(const Path& path, const Bytes& key);
  [[nodiscard]] Result<OperationCost, Error>
  Delete(const Path& path, const Bytes& key, const Transaction& txn);

  /**
   * Delete the element only if it is an empty subtree.
   *
   * @param[in] path  The tree path.
   * @param[in] key   The key to conditionally delete.
   * @return True if deleted, with operation costs.
   */
  [[nodiscard]] Result<Costed<bool>, Error> DeleteIfEmpty(const Path& path, const Bytes& key);
  [[nodiscard]] Result<Costed<bool>, Error>
  DeleteIfEmpty(const Path& path, const Bytes& key, const Transaction& txn);

  /**
   * Delete the element and recursively remove empty parent subtrees.
   *
   * @param[in] path  The tree path.
   * @param[in] key   The key to delete.
   * @return Number of levels removed, with operation costs.
   */
  [[nodiscard]] Result<Costed<uint32_t>, Error>
  PruneEmptyAncestors(const Path& path, const Bytes& key);
  [[nodiscard]] Result<Costed<uint32_t>, Error>
  PruneEmptyAncestors(const Path& path, const Bytes& key, const Transaction& txn);

  /**
   * Remove all elements within the subtree at the given path.
   *
   * @param[in] path  The subtree path to clear.
   * @return True if the subtree was cleared, or an error.
   */
  [[nodiscard]] Result<bool, Error> Clear(const Path& path);
  [[nodiscard]] Result<bool, Error> Clear(const Path& path, const Transaction& txn);

  // -- Query operations -----------------------------------------------------

  /**
   * Execute a query returning raw item values.
   *
   * @param[in] query  The path query to execute.
   * @return Values with skip count and operation costs, or an error.
   */
  [[nodiscard]] Result<Costed<QueryData<std::vector<Bytes>>>, Error>
  QueryValues(const PathQuery& query);
  [[nodiscard]] Result<Costed<QueryData<std::vector<Bytes>>>, Error>
  QueryValues(const PathQuery& query, const Transaction& txn);

  /**
   * Execute a query returning tagged item-or-sum union entries.
   *
   * @param[in] query  The path query to execute.
   * @return Items/sums with skip count and operation costs, or an error.
   */
  [[nodiscard]] Result<Costed<QueryData<std::vector<QueryItemOrSum>>>, Error>
  QueryItemsOrSums(const PathQuery& query);
  [[nodiscard]] Result<Costed<QueryData<std::vector<QueryItemOrSum>>>, Error>
  QueryItemsOrSums(const PathQuery& query, const Transaction& txn);

  /**
   * Execute a query returning i64 sum values only.
   *
   * @param[in] query  The path query to execute.
   * @return Sums with skip count and operation costs, or an error.
   */
  [[nodiscard]] Result<Costed<QueryData<std::vector<int64_t>>>, Error>
  QuerySums(const PathQuery& query);
  [[nodiscard]] Result<Costed<QueryData<std::vector<int64_t>>>, Error>
  QuerySums(const PathQuery& query, const Transaction& txn);

  /**
   * Execute a raw query returning full Element structures.
   *
   * @param[in] query        The path query to execute.
   * @param[in] result_type  0=Element, 1=KeyElementPair, 2=PathKeyElementTrio.
   * @return Result elements with skip count and operation costs, or an error.
   */
  [[nodiscard]] Result<Costed<QueryData<std::vector<QueryResultElement>>>, Error>
  QueryRaw(const PathQuery& query, uint8_t result_type);
  [[nodiscard]] Result<Costed<QueryData<std::vector<QueryResultElement>>>, Error>
  QueryRaw(const PathQuery& query, uint8_t result_type, const Transaction& txn);

  /** Specification for a single query within QueryManyRaw. */
  struct RawQuerySpec {
    const Path& path;
    const std::vector<QueryItem>& items;
    uint32_t limit{0};
    uint32_t offset{0};
  };

  /**
   * Execute multiple raw queries atomically.
   *
   * @param[in] queries      Vector of query specifications.
   * @param[in] result_type  0=Element, 1=KeyElementPair, 2=PathKeyElementTrio.
   * @return Merged result elements with operation costs, or an error.
   */
  [[nodiscard]] Result<Costed<std::vector<QueryResultElement>>, Error>
  QueryManyRaw(const std::vector<RawQuerySpec>& queries, uint8_t result_type);

  /**
   * Execute a query returning path-key-element triples with optional elements.
   *
   * Follows references to resolve element values.
   *
   * @param[in] query  The path query to execute.
   * @return Path-key-element triples with operation costs, or an error.
   */
  [[nodiscard]] Result<Costed<std::vector<PathKeyElement>>, Error>
  QueryKeysOptional(const PathQuery& query);
  [[nodiscard]] Result<Costed<std::vector<PathKeyElement>>, Error>
  QueryKeysOptional(const PathQuery& query, const Transaction& txn);

  /**
   * Execute a raw query returning path-key-element triples (no reference following).
   *
   * @param[in] query  The path query to execute.
   * @return Path-key-element triples with operation costs, or an error.
   */
  [[nodiscard]] Result<Costed<std::vector<PathKeyElement>>, Error>
  QueryRawKeysOptional(const PathQuery& query);
  [[nodiscard]] Result<Costed<std::vector<PathKeyElement>>, Error>
  QueryRawKeysOptional(const PathQuery& query, const Transaction& txn);

  // -- Proof operations -----------------------------------------------------

  /** Result of a proof verification. */
  struct ProofVerifyResult {
    Hash m_root_hash; /**< 32-byte Merkle root. */
    std::vector<PathKeyElement> m_entries; /**< Path-key-optional-element triples. */
  };

  /** Result of chained proof verification. */
  struct ChainedVerifyResult {
    Hash m_root_hash; /**< Final 32-byte Merkle root. */
    std::vector<std::vector<PathKeyElement>> m_query_results; /**< One result set per query. */
  };

  /**
   * Generate a proof for a path query.
   *
   * @param[in] query                    The path query to prove.
   * @param[in] decrease_limit_on_empty  ProveOptions flag (default true).
   * @return Serialized proof bytes with operation costs, or an error.
   */
  [[nodiscard]] Result<Costed<Bytes>, Error>
  Prove(const PathQuery& query, bool decrease_limit_on_empty = true);

  /**
   * Verify a proof with strict succinctness.
   *
   * @param[in] proof  Serialized proof bytes.
   * @param[in] query  The path query that was proved.
   * @return Verification result with root hash and entries.
   */
  [[nodiscard]] Result<Costed<ProofVerifyResult>, Error>
  VerifyQuery(const Bytes& proof, const PathQuery& query);

  /**
   * Verify a proof with custom options.
   *
   * @param[in] proof                Serialized proof bytes.
   * @param[in] query                The path query that was proved.
   * @param[in] absence_proofs       Enable absence proofs for non-existing keys.
   * @param[in] verify_succinctness  Verify proof succinctness.
   * @param[in] include_empty_trees  Include empty trees in results.
   * @return Verification result.
   */
  [[nodiscard]] Result<Costed<ProofVerifyResult>, Error> VerifyQueryWithOptions(
      const Bytes& proof,
      const PathQuery& query,
      bool absence_proofs,
      bool verify_succinctness,
      bool include_empty_trees
  );

  /**
   * Verify a subset query proof (non-strict succinctness).
   *
   * @param[in] proof  Serialized proof bytes.
   * @param[in] query  The path query.
   * @return Verification result.
   */
  [[nodiscard]] Result<Costed<ProofVerifyResult>, Error>
  VerifySubsetQuery(const Bytes& proof, const PathQuery& query);

  /**
   * Verify a proof with absence proofs for non-existing searched keys.
   *
   * @param[in] proof  Serialized proof bytes.
   * @param[in] query  The path query (must have a limit set).
   * @return Verification result.
   */
  [[nodiscard]] Result<Costed<ProofVerifyResult>, Error>
  VerifyQueryWithAbsenceProof(const Bytes& proof, const PathQuery& query);

  /**
   * Verify a subset query proof with absence proofs.
   *
   * @param[in] proof  Serialized proof bytes.
   * @param[in] query  The path query.
   * @return Verification result.
   */
  [[nodiscard]] Result<Costed<ProofVerifyResult>, Error>
  VerifySubsetQueryWithAbsenceProof(const Bytes& proof, const PathQuery& query);

  /**
   * Verify a proof against a sequence of chained queries.
   *
   * The first query is verified against the proof.  For each subsequent
   * query, the first result key of the previous query is appended to
   * the chained query's path before verification.
   *
   * @param[in] proof            Serialized proof bytes.
   * @param[in] first_query      The initial path query.
   * @param[in] chained_queries  Pointers to subsequent queries.
   * @return Chained verification result with per-query result sets.
   */
  [[nodiscard]] Result<Costed<ChainedVerifyResult>, Error> VerifyChainedQueries(
      const Bytes& proof,
      const PathQuery& first_query,
      const std::vector<PathQuery*>& chained_queries
  );

  // -- Batch operations -----------------------------------------------------

  /**
   * Apply a batch of operations atomically.
   *
   * All operations within the batch are validated and applied in a single
   * atomic step. If any operation fails, none are applied.
   *
   * @param[in] ops      The batch operations to apply.
   * @param[in] options  Options controlling validation behavior.
   * @return Operation costs, or an error.
   */
  [[nodiscard]] Result<OperationCost, Error>
  ApplyBatch(const std::vector<BatchOperation>& ops, const BatchApplyOptions& options);

  /**
   * Apply a batch of operations atomically within a transaction.
   *
   * @param[in] ops      The batch operations to apply.
   * @param[in] options  Options controlling validation behavior.
   * @param[in] txn      The transaction.
   * @return Operation costs, or an error.
   */
  [[nodiscard]] Result<OperationCost, Error> ApplyBatch(
      const std::vector<BatchOperation>& ops,
      const BatchApplyOptions& options,
      const Transaction& txn
  );

private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
};

/** Return a human-readable build identification string. */
[[nodiscard]] std::string GetWhoami();
} // namespace grovedb

#endif // LIBGROVEDB_DB_H

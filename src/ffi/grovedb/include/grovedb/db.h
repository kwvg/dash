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

namespace grovedb {
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

private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
};

/** Return a human-readable build identification string. */
[[nodiscard]] std::string GetWhoami();
} // namespace grovedb

#endif // LIBGROVEDB_DB_H

// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GROVEDB_DB_H
#define GROVEDB_DB_H

#include <grovedb/cost.h>
#include <grovedb/element.h>
#include <grovedb/status.h>
#include <grovedb/transaction.h>
#include <grovedb/types.h>

#include <memory>
#include <optional>
#include <string>

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

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

/** Return a human-readable build identification string. */
[[nodiscard]] std::string GetWhoami();
} // namespace grovedb

#endif // GROVEDB_DB_H

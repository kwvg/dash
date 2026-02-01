//
// Copyright (c) 2026-present, The Dash Core developers
// SPDX-License-Identifier: MIT
// See the accompanying file LICENSE or https://opensource.org/license/mit
//

pub use grovedb;

mod built_info {
  include!(concat!(env!("OUT_DIR"), "/built.rs"));
}

mod lifecycle;
use lifecycle::{grovedb_flush, grovedb_open, grovedb_root_hash, grovedb_verify, grovedb_wipe};

mod transaction;
use transaction::{grovedb_commit_transaction, grovedb_rollback_transaction, grovedb_start_transaction};

mod types;

mod element;
use element::{
    grovedb_element_empty_sum_tree, grovedb_element_empty_tree, grovedb_element_item,
    grovedb_element_sum_item,
};

mod get;
use get::{
    grovedb_check_subtree_exists, grovedb_check_subtree_exists_with_tx, grovedb_get,
    grovedb_get_raw, grovedb_get_raw_optional, grovedb_get_raw_optional_with_tx,
    grovedb_get_raw_with_tx, grovedb_get_with_tx, grovedb_has_raw, grovedb_has_raw_with_tx,
};

mod insert;
use insert::{
    grovedb_insert, grovedb_insert_if_changed_value, grovedb_insert_if_changed_value_with_tx,
    grovedb_insert_if_not_exists, grovedb_insert_if_not_exists_return_existing,
    grovedb_insert_if_not_exists_return_existing_with_tx, grovedb_insert_if_not_exists_with_tx,
    grovedb_insert_with_tx,
};

mod delete;
use delete::{
    grovedb_clear_subtree, grovedb_clear_subtree_with_tx, grovedb_delete,
    grovedb_delete_if_empty_tree, grovedb_delete_if_empty_tree_with_tx,
    grovedb_delete_up_tree_while_empty, grovedb_delete_up_tree_while_empty_with_tx,
    grovedb_delete_with_tx,
};

mod proof;
use proof::{
    grovedb_prove_query, grovedb_verify_query,
    grovedb_verify_query_with_absence_proof, grovedb_verify_query_with_options,
    grovedb_verify_subset_query, grovedb_verify_subset_query_with_absence_proof,
};

mod query;
use query::{
    grovedb_path_query_new, grovedb_path_query_new_with_subquery,
    grovedb_query_item_value, grovedb_query_item_value_with_tx,
    grovedb_query_item_value_or_sum, grovedb_query_item_value_or_sum_with_tx,
    grovedb_query_keys_optional, grovedb_query_keys_optional_with_tx,
    grovedb_query_many_raw,
    grovedb_query_raw, grovedb_query_raw_with_tx,
    grovedb_query_raw_keys_optional, grovedb_query_raw_keys_optional_with_tx,
    grovedb_query_sums, grovedb_query_sums_with_tx,
};

mod batch;
use batch::{grovedb_apply_batch, grovedb_apply_batch_with_tx};

/// Opaque wrapper around `grovedb::GroveDb` for use across the CXX bridge.
pub struct BoxedGroveDb {
  db: grovedb::GroveDb,
}

impl std::fmt::Debug for BoxedGroveDb {
  fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
    f.debug_struct("BoxedGroveDb").finish_non_exhaustive()
  }
}

/// Opaque wrapper around a `grovedb::Transaction` with an erased lifetime.
///
/// # Safety
///
/// The `'static` lifetime is a lie — the C++ `Transaction` RAII wrapper
/// enforces the invariant that this never outlives the `BoxedGroveDb` that
/// created it.
#[allow(unsafe_code)]
pub struct BoxedTransaction {
  tx: grovedb::Transaction<'static>,
}

impl std::fmt::Debug for BoxedTransaction {
  fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
    f.debug_struct("BoxedTransaction").finish_non_exhaustive()
  }
}

/// Opaque wrapper around a `grovedb::PathQuery` for use across the CXX bridge.
pub struct BoxedPathQuery {
  query: grovedb::PathQuery,
}

impl std::fmt::Debug for BoxedPathQuery {
  fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
    f.debug_struct("BoxedPathQuery").finish_non_exhaustive()
  }
}

#[allow(unsafe_code)]
#[cxx::bridge(namespace = "grovedb_cxx")]
pub(crate) mod ffi {
  /// Operation cost returned alongside every cost-tracked result.
  struct FfiOperationCost {
    seek_count: u32,
    storage_added_bytes: u32,
    storage_replaced_bytes: u32,
    storage_removed_bytes: u32,
    storage_loaded_bytes: u64,
    hash_node_calls: u32,
  }

  /// Result of `root_hash`: the 32-byte Merkle root plus operation costs.
  struct FfiRootHashResult {
    root_hash: Vec<u8>,
    cost: FfiOperationCost,
  }

  /// Result of a get operation: serialized element plus operation costs.
  struct FfiElementResult {
    element: Vec<u8>,
    cost: FfiOperationCost,
  }

  /// Result of an optional get: flag + optional serialized element + costs.
  struct FfiOptionalElementResult {
    has_element: bool,
    element: Vec<u8>,
    cost: FfiOperationCost,
  }

  /// Result of a boolean query (existence check) plus operation costs.
  struct FfiBoolResult {
    value: bool,
    cost: FfiOperationCost,
  }

  /// Result of `insert_if_changed_value`: whether the value changed, the
  /// optional previous element, and operation costs.
  struct FfiChangedValueResult {
    changed: bool,
    has_previous_element: bool,
    previous_element: Vec<u8>,
    cost: FfiOperationCost,
  }

  /// Result of an operation that returns a count (u32) plus operation costs.
  struct FfiU32Result {
    value: u32,
    cost: FfiOperationCost,
  }

  /// Result of a query_item_value operation: wire-encoded values, skipped
  /// count, and operation costs.
  struct FfiQueryResult {
    /// Wire-encoded values: `[u32 count][u32 len₁][bytes₁]…`
    values: Vec<u8>,
    skipped: u16,
    cost: FfiOperationCost,
  }

  /// Result of a query_item_value_or_sum operation: wire-encoded tagged union
  /// entries, skipped count, and operation costs.
  struct FfiQueryItemOrSumResult {
    /// Wire-encoded: `[u32 count][u8 tag + data]…`
    values: Vec<u8>,
    skipped: u16,
    cost: FfiOperationCost,
  }

  /// Result of a query_sums operation: wire-encoded i64 sum values.
  struct FfiQuerySumsResult {
    /// Wire-encoded: `[u32 count][i64₁ le]…`
    values: Vec<u8>,
    skipped: u16,
    cost: FfiOperationCost,
  }

  /// Result of a query_raw or query_many_raw operation: wire-encoded
  /// QueryResultElement entries.
  struct FfiQueryRawResult {
    /// Wire-encoded: `[u32 count][u8 variant + data]…`
    values: Vec<u8>,
    skipped: u16,
    cost: FfiOperationCost,
  }

  /// Result of a query_keys_optional or query_raw_keys_optional operation:
  /// wire-encoded PathKeyOptionalElementTrio entries.
  struct FfiQueryKeysOptionalResult {
    /// Wire-encoded: `[u32 count][path][key][has_elem][opt elem]…`
    values: Vec<u8>,
    cost: FfiOperationCost,
  }

  /// Result of proof generation: opaque proof bytes plus operation costs.
  struct FfiProofResult {
    proof: Vec<u8>,
    cost: FfiOperationCost,
  }

  /// Result of proof verification: root hash and wire-encoded entries.
  struct FfiVerifyResult {
    /// 32-byte Merkle root hash extracted from the proof.
    root_hash: Vec<u8>,
    /// Wire-encoded `PathKeyOptionalElementTrio` entries.
    entries: Vec<u8>,
  }

  /// Options controlling batch application behavior.
  struct FfiBatchApplyOptions {
    validate_insertion_does_not_override: bool,
    validate_insertion_does_not_override_tree: bool,
    allow_deleting_non_empty_trees: bool,
    deleting_non_empty_trees_returns_error: bool,
    disable_operation_consistency_check: bool,
    base_root_storage_is_free: bool,
  }

  extern "Rust" {
    type BoxedGroveDb;
    type BoxedTransaction;
    type BoxedPathQuery;

    fn whoami() -> String;

    // -- Lifecycle --
    fn grovedb_open(path: &str) -> Result<Box<BoxedGroveDb>>;
    fn grovedb_flush(db: &BoxedGroveDb) -> Result<()>;
    fn grovedb_wipe(db: &BoxedGroveDb) -> Result<()>;
    fn grovedb_root_hash(db: &BoxedGroveDb) -> Result<FfiRootHashResult>;
    fn grovedb_verify(db: &BoxedGroveDb) -> Result<bool>;

    // -- Transactions --
    fn grovedb_start_transaction(db: &BoxedGroveDb) -> Result<Box<BoxedTransaction>>;
    fn grovedb_commit_transaction(db: &BoxedGroveDb, tx: Box<BoxedTransaction>) -> Result<FfiOperationCost>;
    fn grovedb_rollback_transaction(db: &BoxedGroveDb, tx: &BoxedTransaction) -> Result<()>;

    // -- Element factories --
    fn grovedb_element_item(value: &[u8]) -> Result<Vec<u8>>;
    fn grovedb_element_empty_tree() -> Result<Vec<u8>>;
    fn grovedb_element_empty_sum_tree() -> Result<Vec<u8>>;
    fn grovedb_element_sum_item(value: i64) -> Result<Vec<u8>>;

    // -- Get (follows references) --
    fn grovedb_get(db: &BoxedGroveDb, path: &[u8], key: &[u8]) -> Result<FfiElementResult>;
    fn grovedb_get_with_tx(db: &BoxedGroveDb, path: &[u8], key: &[u8], tx: &BoxedTransaction) -> Result<FfiElementResult>;

    // -- Get raw (no reference following) --
    fn grovedb_get_raw(db: &BoxedGroveDb, path: &[u8], key: &[u8]) -> Result<FfiElementResult>;
    fn grovedb_get_raw_with_tx(db: &BoxedGroveDb, path: &[u8], key: &[u8], tx: &BoxedTransaction) -> Result<FfiElementResult>;

    // -- Get raw optional --
    fn grovedb_get_raw_optional(db: &BoxedGroveDb, path: &[u8], key: &[u8]) -> Result<FfiOptionalElementResult>;
    fn grovedb_get_raw_optional_with_tx(db: &BoxedGroveDb, path: &[u8], key: &[u8], tx: &BoxedTransaction) -> Result<FfiOptionalElementResult>;

    // -- Has raw (existence check) --
    fn grovedb_has_raw(db: &BoxedGroveDb, path: &[u8], key: &[u8]) -> Result<FfiBoolResult>;
    fn grovedb_has_raw_with_tx(db: &BoxedGroveDb, path: &[u8], key: &[u8], tx: &BoxedTransaction) -> Result<FfiBoolResult>;

    // -- Subtree exists --
    fn grovedb_check_subtree_exists(db: &BoxedGroveDb, path: &[u8]) -> Result<FfiBoolResult>;
    fn grovedb_check_subtree_exists_with_tx(db: &BoxedGroveDb, path: &[u8], tx: &BoxedTransaction) -> Result<FfiBoolResult>;

    // -- Insert (unconditional) --
    fn grovedb_insert(db: &BoxedGroveDb, path: &[u8], key: &[u8], element: &[u8]) -> Result<FfiOperationCost>;
    fn grovedb_insert_with_tx(db: &BoxedGroveDb, path: &[u8], key: &[u8], element: &[u8], tx: &BoxedTransaction) -> Result<FfiOperationCost>;

    // -- Insert if not exists --
    fn grovedb_insert_if_not_exists(db: &BoxedGroveDb, path: &[u8], key: &[u8], element: &[u8]) -> Result<FfiBoolResult>;
    fn grovedb_insert_if_not_exists_with_tx(db: &BoxedGroveDb, path: &[u8], key: &[u8], element: &[u8], tx: &BoxedTransaction) -> Result<FfiBoolResult>;

    // -- Insert if not exists, return existing --
    fn grovedb_insert_if_not_exists_return_existing(db: &BoxedGroveDb, path: &[u8], key: &[u8], element: &[u8]) -> Result<FfiOptionalElementResult>;
    fn grovedb_insert_if_not_exists_return_existing_with_tx(db: &BoxedGroveDb, path: &[u8], key: &[u8], element: &[u8], tx: &BoxedTransaction) -> Result<FfiOptionalElementResult>;

    // -- Insert if changed value --
    fn grovedb_insert_if_changed_value(db: &BoxedGroveDb, path: &[u8], key: &[u8], element: &[u8]) -> Result<FfiChangedValueResult>;
    fn grovedb_insert_if_changed_value_with_tx(db: &BoxedGroveDb, path: &[u8], key: &[u8], element: &[u8], tx: &BoxedTransaction) -> Result<FfiChangedValueResult>;

    // -- Delete (unconditional) --
    fn grovedb_delete(db: &BoxedGroveDb, path: &[u8], key: &[u8]) -> Result<FfiOperationCost>;
    fn grovedb_delete_with_tx(db: &BoxedGroveDb, path: &[u8], key: &[u8], tx: &BoxedTransaction) -> Result<FfiOperationCost>;

    // -- Delete if empty tree --
    fn grovedb_delete_if_empty_tree(db: &BoxedGroveDb, path: &[u8], key: &[u8]) -> Result<FfiBoolResult>;
    fn grovedb_delete_if_empty_tree_with_tx(db: &BoxedGroveDb, path: &[u8], key: &[u8], tx: &BoxedTransaction) -> Result<FfiBoolResult>;

    // -- Delete up tree while empty (prune ancestors) --
    fn grovedb_delete_up_tree_while_empty(db: &BoxedGroveDb, path: &[u8], key: &[u8]) -> Result<FfiU32Result>;
    fn grovedb_delete_up_tree_while_empty_with_tx(db: &BoxedGroveDb, path: &[u8], key: &[u8], tx: &BoxedTransaction) -> Result<FfiU32Result>;

    // -- Clear subtree --
    fn grovedb_clear_subtree(db: &BoxedGroveDb, path: &[u8]) -> Result<bool>;
    fn grovedb_clear_subtree_with_tx(db: &BoxedGroveDb, path: &[u8], tx: &BoxedTransaction) -> Result<bool>;

    // -- PathQuery factories --
    fn grovedb_path_query_new(path: &[u8], query_items: &[u8], limit: u32, offset: u32) -> Result<Box<BoxedPathQuery>>;
    fn grovedb_path_query_new_with_subquery(path: &[u8], query_items: &[u8], limit: u32, offset: u32, subquery_path: &[u8], subquery_items: &[u8]) -> Result<Box<BoxedPathQuery>>;

    // -- Query item value --
    fn grovedb_query_item_value(db: &BoxedGroveDb, query: &BoxedPathQuery) -> Result<FfiQueryResult>;
    fn grovedb_query_item_value_with_tx(db: &BoxedGroveDb, query: &BoxedPathQuery, tx: &BoxedTransaction) -> Result<FfiQueryResult>;

    // -- Query item value or sum --
    fn grovedb_query_item_value_or_sum(db: &BoxedGroveDb, query: &BoxedPathQuery) -> Result<FfiQueryItemOrSumResult>;
    fn grovedb_query_item_value_or_sum_with_tx(db: &BoxedGroveDb, query: &BoxedPathQuery, tx: &BoxedTransaction) -> Result<FfiQueryItemOrSumResult>;

    // -- Query sums --
    fn grovedb_query_sums(db: &BoxedGroveDb, query: &BoxedPathQuery) -> Result<FfiQuerySumsResult>;
    fn grovedb_query_sums_with_tx(db: &BoxedGroveDb, query: &BoxedPathQuery, tx: &BoxedTransaction) -> Result<FfiQuerySumsResult>;

    // -- Query raw --
    fn grovedb_query_raw(db: &BoxedGroveDb, query: &BoxedPathQuery, result_type: u8) -> Result<FfiQueryRawResult>;
    fn grovedb_query_raw_with_tx(db: &BoxedGroveDb, query: &BoxedPathQuery, result_type: u8, tx: &BoxedTransaction) -> Result<FfiQueryRawResult>;

    // -- Query many raw --
    fn grovedb_query_many_raw(db: &BoxedGroveDb, encoded_queries: &[u8], result_type: u8) -> Result<FfiQueryRawResult>;

    // -- Query keys optional --
    fn grovedb_query_keys_optional(db: &BoxedGroveDb, query: &BoxedPathQuery) -> Result<FfiQueryKeysOptionalResult>;
    fn grovedb_query_keys_optional_with_tx(db: &BoxedGroveDb, query: &BoxedPathQuery, tx: &BoxedTransaction) -> Result<FfiQueryKeysOptionalResult>;

    // -- Query raw keys optional --
    fn grovedb_query_raw_keys_optional(db: &BoxedGroveDb, query: &BoxedPathQuery) -> Result<FfiQueryKeysOptionalResult>;
    fn grovedb_query_raw_keys_optional_with_tx(db: &BoxedGroveDb, query: &BoxedPathQuery, tx: &BoxedTransaction) -> Result<FfiQueryKeysOptionalResult>;

    // -- Proofs --
    fn grovedb_prove_query(db: &BoxedGroveDb, query: &BoxedPathQuery, decrease_limit_on_empty: bool) -> Result<FfiProofResult>;
    fn grovedb_verify_query(proof: &[u8], query: &BoxedPathQuery) -> Result<FfiVerifyResult>;
    fn grovedb_verify_query_with_options(proof: &[u8], query: &BoxedPathQuery, absence_proofs: bool, verify_succinctness: bool, include_empty_trees: bool) -> Result<FfiVerifyResult>;
    fn grovedb_verify_subset_query(proof: &[u8], query: &BoxedPathQuery) -> Result<FfiVerifyResult>;
    fn grovedb_verify_query_with_absence_proof(proof: &[u8], query: &BoxedPathQuery) -> Result<FfiVerifyResult>;
    fn grovedb_verify_subset_query_with_absence_proof(proof: &[u8], query: &BoxedPathQuery) -> Result<FfiVerifyResult>;

    // -- Batch --
    fn grovedb_apply_batch(db: &BoxedGroveDb, ops: &[u8], options: &FfiBatchApplyOptions) -> Result<FfiOperationCost>;
    fn grovedb_apply_batch_with_tx(db: &BoxedGroveDb, ops: &[u8], options: &FfiBatchApplyOptions, tx: &BoxedTransaction) -> Result<FfiOperationCost>;
  }
}

fn whoami() -> String {
  format!("{} {} built with {}", built_info::PKG_NAME, built_info::PKG_VERSION, built_info::RUSTC_VERSION)
}

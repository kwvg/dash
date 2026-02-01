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
use element::{grovedb_element_empty_tree, grovedb_element_item};

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

mod query;
use query::{
    grovedb_path_query_new, grovedb_path_query_new_with_subquery,
    grovedb_query_item_value, grovedb_query_item_value_with_tx,
};

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
  }
}

fn whoami() -> String {
  format!("{} {} built with {}", built_info::PKG_NAME, built_info::PKG_VERSION, built_info::RUSTC_VERSION)
}

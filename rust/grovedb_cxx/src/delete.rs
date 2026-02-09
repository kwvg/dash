//
// Copyright (c) 2026-present, The Dash Core developers
// SPDX-License-Identifier: MIT
// See the accompanying file LICENSE or https://opensource.org/license/mit
//

use grovedb::operations::delete::DeleteUpTreeOptions;
use grovedb_version::version::GroveVersion;

use crate::ffi::{FfiBoolResult, FfiOperationCost, FfiU32Result};
use crate::lifecycle::operation_cost_to_ffi;
use crate::types::decode_path;
use crate::BoxedGroveDb;
use crate::BoxedTransaction;

// ---------------------------------------------------------------------------
// delete — unconditional delete
// ---------------------------------------------------------------------------

/// Delete the element at the given path and key.
pub fn grovedb_delete(
  db: &BoxedGroveDb,
  path: &[u8],
  key: &[u8],
) -> Result<FfiOperationCost, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let ctx = db
    .db
    .delete(segments.as_slice(), key, None, None, version);
  let cost = operation_cost_to_ffi(&ctx.cost);
  ctx.value.map_err(crate::ffi_error)?;
  Ok(cost)
}

/// Delete the element at the given path and key within a transaction.
pub fn grovedb_delete_with_tx(
  db: &BoxedGroveDb,
  path: &[u8],
  key: &[u8],
  tx: &BoxedTransaction,
) -> Result<FfiOperationCost, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let ctx = db
    .db
    .delete(segments.as_slice(), key, None, Some(&tx.tx), version);
  let cost = operation_cost_to_ffi(&ctx.cost);
  ctx.value.map_err(crate::ffi_error)?;
  Ok(cost)
}

// ---------------------------------------------------------------------------
// delete_if_empty_tree — delete only if the target is an empty tree
// ---------------------------------------------------------------------------

/// Delete the element only if it is an empty subtree.
///
/// Returns `value = true` if the element was deleted, `false` if the
/// element was not an empty tree or did not exist.
pub fn grovedb_delete_if_empty_tree(
  db: &BoxedGroveDb,
  path: &[u8],
  key: &[u8],
) -> Result<FfiBoolResult, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let ctx = db
    .db
    .delete_if_empty_tree(segments.as_slice(), key, None, version);
  let cost = operation_cost_to_ffi(&ctx.cost);
  let deleted = ctx.value.map_err(crate::ffi_error)?;
  Ok(FfiBoolResult {
    value: deleted,
    cost,
  })
}

/// Delete the element only if it is an empty subtree, within a transaction.
pub fn grovedb_delete_if_empty_tree_with_tx(
  db: &BoxedGroveDb,
  path: &[u8],
  key: &[u8],
  tx: &BoxedTransaction,
) -> Result<FfiBoolResult, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let ctx = db.db.delete_if_empty_tree(
    segments.as_slice(),
    key,
    Some(&tx.tx),
    version,
  );
  let cost = operation_cost_to_ffi(&ctx.cost);
  let deleted = ctx.value.map_err(crate::ffi_error)?;
  Ok(FfiBoolResult {
    value: deleted,
    cost,
  })
}

// ---------------------------------------------------------------------------
// delete_up_tree_while_empty — cascade delete empty ancestors
// ---------------------------------------------------------------------------

/// Delete the element and recursively remove empty parent subtrees.
///
/// Returns the number of levels removed (as a u32).
pub fn grovedb_delete_up_tree_while_empty(
  db: &BoxedGroveDb,
  path: &[u8],
  key: &[u8],
) -> Result<FfiU32Result, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let options = DeleteUpTreeOptions {
    stop_path_height: Some(0),
    ..DeleteUpTreeOptions::default()
  };
  let ctx = db.db.delete_up_tree_while_empty(
    segments.as_slice(),
    key,
    &options,
    None,
    version,
  );
  let cost = operation_cost_to_ffi(&ctx.cost);
  let count = ctx.value.map_err(crate::ffi_error)?;
  Ok(FfiU32Result {
    value: u32::from(count),
    cost,
  })
}

/// Delete the element and recursively remove empty parent subtrees,
/// within a transaction.
pub fn grovedb_delete_up_tree_while_empty_with_tx(
  db: &BoxedGroveDb,
  path: &[u8],
  key: &[u8],
  tx: &BoxedTransaction,
) -> Result<FfiU32Result, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let options = DeleteUpTreeOptions {
    stop_path_height: Some(0),
    ..DeleteUpTreeOptions::default()
  };
  let ctx = db.db.delete_up_tree_while_empty(
    segments.as_slice(),
    key,
    &options,
    Some(&tx.tx),
    version,
  );
  let cost = operation_cost_to_ffi(&ctx.cost);
  let count = ctx.value.map_err(crate::ffi_error)?;
  Ok(FfiU32Result {
    value: u32::from(count),
    cost,
  })
}

// ---------------------------------------------------------------------------
// clear_subtree — remove all elements in a subtree
// ---------------------------------------------------------------------------

/// Remove all elements within the subtree at the given path.
pub fn grovedb_clear_subtree(
  db: &BoxedGroveDb,
  path: &[u8],
) -> Result<bool, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  db.db
    .clear_subtree(segments.as_slice(), None, None, version)
    .map_err(crate::ffi_error)
}

/// Remove all elements within the subtree, within a transaction.
pub fn grovedb_clear_subtree_with_tx(
  db: &BoxedGroveDb,
  path: &[u8],
  tx: &BoxedTransaction,
) -> Result<bool, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  db.db
    .clear_subtree(segments.as_slice(), None, Some(&tx.tx), version)
    .map_err(crate::ffi_error)
}

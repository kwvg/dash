//
// Copyright (c) 2026-present, The Dash Core developers
// SPDX-License-Identifier: MIT
// See the accompanying file LICENSE or https://opensource.org/license/mit
//

use grovedb_path::SubtreePath;
use grovedb_version::version::GroveVersion;

use crate::element::serialize_element;
use crate::ffi::{FfiBoolResult, FfiElementResult, FfiOptionalElementResult};
use crate::lifecycle::operation_cost_to_ffi;
use crate::types::decode_path;
use crate::BoxedGroveDb;
use crate::BoxedTransaction;

// ---------------------------------------------------------------------------
// get — follows references
// ---------------------------------------------------------------------------

/// Get an element by path and key, following any references to their target.
pub fn grovedb_get(
  db: &BoxedGroveDb,
  path: &[u8],
  key: &[u8],
) -> Result<FfiElementResult, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let ctx = db.db.get(segments.as_slice(), key, None, version);
  let cost = operation_cost_to_ffi(&ctx.cost);
  let element = ctx.value.map_err(crate::ffi_error)?;
  Ok(FfiElementResult {
    element: serialize_element(&element, version)?,
    cost,
  })
}

/// Get an element by path and key within a transaction, following references.
pub fn grovedb_get_with_tx(
  db: &BoxedGroveDb,
  path: &[u8],
  key: &[u8],
  tx: &BoxedTransaction,
) -> Result<FfiElementResult, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let ctx = db
    .db
    .get(segments.as_slice(), key, Some(&tx.tx), version);
  let cost = operation_cost_to_ffi(&ctx.cost);
  let element = ctx.value.map_err(crate::ffi_error)?;
  Ok(FfiElementResult {
    element: serialize_element(&element, version)?,
    cost,
  })
}

// ---------------------------------------------------------------------------
// get_raw — no reference following
// ---------------------------------------------------------------------------

/// Get an element by path and key without following references.
pub fn grovedb_get_raw(
  db: &BoxedGroveDb,
  path: &[u8],
  key: &[u8],
) -> Result<FfiElementResult, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let ctx = db
    .db
    .get_raw(segments.as_slice().into(), key, None, version);
  let cost = operation_cost_to_ffi(&ctx.cost);
  let element = ctx.value.map_err(crate::ffi_error)?;
  Ok(FfiElementResult {
    element: serialize_element(&element, version)?,
    cost,
  })
}

/// Get an element by path and key within a transaction, without following references.
pub fn grovedb_get_raw_with_tx(
  db: &BoxedGroveDb,
  path: &[u8],
  key: &[u8],
  tx: &BoxedTransaction,
) -> Result<FfiElementResult, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let ctx = db
    .db
    .get_raw(segments.as_slice().into(), key, Some(&tx.tx), version);
  let cost = operation_cost_to_ffi(&ctx.cost);
  let element = ctx.value.map_err(crate::ffi_error)?;
  Ok(FfiElementResult {
    element: serialize_element(&element, version)?,
    cost,
  })
}

// ---------------------------------------------------------------------------
// get_raw_optional — returns None instead of erroring when missing
// ---------------------------------------------------------------------------

/// Get an element optionally: returns `has_element = false` instead of an
/// error when the key is missing.
pub fn grovedb_get_raw_optional(
  db: &BoxedGroveDb,
  path: &[u8],
  key: &[u8],
) -> Result<FfiOptionalElementResult, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let ctx = db
    .db
    .get_raw_optional(segments.as_slice().into(), key, None, version);
  let cost = operation_cost_to_ffi(&ctx.cost);
  let maybe = ctx.value.map_err(crate::ffi_error)?;
  match maybe {
    Some(element) => Ok(FfiOptionalElementResult {
      has_element: true,
      element: serialize_element(&element, version)?,
      cost,
    }),
    None => Ok(FfiOptionalElementResult {
      has_element: false,
      element: Vec::new(),
      cost,
    }),
  }
}

/// Get an element optionally within a transaction.
pub fn grovedb_get_raw_optional_with_tx(
  db: &BoxedGroveDb,
  path: &[u8],
  key: &[u8],
  tx: &BoxedTransaction,
) -> Result<FfiOptionalElementResult, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let ctx = db
    .db
    .get_raw_optional(segments.as_slice().into(), key, Some(&tx.tx), version);
  let cost = operation_cost_to_ffi(&ctx.cost);
  let maybe = ctx.value.map_err(crate::ffi_error)?;
  match maybe {
    Some(element) => Ok(FfiOptionalElementResult {
      has_element: true,
      element: serialize_element(&element, version)?,
      cost,
    }),
    None => Ok(FfiOptionalElementResult {
      has_element: false,
      element: Vec::new(),
      cost,
    }),
  }
}

// ---------------------------------------------------------------------------
// has_raw — existence check
// ---------------------------------------------------------------------------

/// Check whether a key exists at the given path (no reference following).
pub fn grovedb_has_raw(
  db: &BoxedGroveDb,
  path: &[u8],
  key: &[u8],
) -> Result<FfiBoolResult, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let ctx = db.db.has_raw(segments.as_slice(), key, None, version);
  let cost = operation_cost_to_ffi(&ctx.cost);
  let exists = ctx.value.map_err(crate::ffi_error)?;
  Ok(FfiBoolResult {
    value: exists,
    cost,
  })
}

/// Check whether a key exists at the given path within a transaction.
pub fn grovedb_has_raw_with_tx(
  db: &BoxedGroveDb,
  path: &[u8],
  key: &[u8],
  tx: &BoxedTransaction,
) -> Result<FfiBoolResult, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let ctx = db
    .db
    .has_raw(segments.as_slice(), key, Some(&tx.tx), version);
  let cost = operation_cost_to_ffi(&ctx.cost);
  let exists = ctx.value.map_err(crate::ffi_error)?;
  Ok(FfiBoolResult {
    value: exists,
    cost,
  })
}

// ---------------------------------------------------------------------------
// check_subtree_exists — subtree/path validation
// ---------------------------------------------------------------------------

/// Check whether all parent subtrees in the path exist.
///
/// Returns `value = true` when the path is valid, `value = false` when any
/// parent subtree is missing.  Only returns `Err` for path-encoding errors.
pub fn grovedb_check_subtree_exists(
  db: &BoxedGroveDb,
  path: &[u8],
) -> Result<FfiBoolResult, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let ctx = db
    .db
    .check_subtree_exists_invalid_path(segments.as_slice().into(), None, version);
  let cost = operation_cost_to_ffi(&ctx.cost);
  Ok(FfiBoolResult {
    value: ctx.value.is_ok(),
    cost,
  })
}

/// Check whether all parent subtrees in the path exist, within a transaction.
pub fn grovedb_check_subtree_exists_with_tx(
  db: &BoxedGroveDb,
  path: &[u8],
  tx: &BoxedTransaction,
) -> Result<FfiBoolResult, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let ctx = db.db.check_subtree_exists_invalid_path(
    segments.as_slice().into(),
    Some(&tx.tx),
    version,
  );
  let cost = operation_cost_to_ffi(&ctx.cost);
  Ok(FfiBoolResult {
    value: ctx.value.is_ok(),
    cost,
  })
}

// ---------------------------------------------------------------------------
// is_empty_tree — check whether a subtree is empty
// ---------------------------------------------------------------------------

/// Check whether the subtree at the given path is empty.
pub fn grovedb_is_empty_tree(
  db: &BoxedGroveDb,
  path: &[u8],
) -> Result<FfiBoolResult, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let subtree_path: SubtreePath<Vec<u8>> = segments.as_slice().into();
  let ctx = db.db.is_empty_tree(subtree_path, None, version);
  let cost = operation_cost_to_ffi(&ctx.cost);
  let empty = ctx.value.map_err(crate::ffi_error)?;
  Ok(FfiBoolResult {
    value: empty,
    cost,
  })
}

/// Check whether the subtree at the given path is empty, within a transaction.
pub fn grovedb_is_empty_tree_with_tx(
  db: &BoxedGroveDb,
  path: &[u8],
  tx: &BoxedTransaction,
) -> Result<FfiBoolResult, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let subtree_path: SubtreePath<Vec<u8>> = segments.as_slice().into();
  let ctx = db.db.is_empty_tree(subtree_path, Some(&tx.tx), version);
  let cost = operation_cost_to_ffi(&ctx.cost);
  let empty = ctx.value.map_err(crate::ffi_error)?;
  Ok(FfiBoolResult {
    value: empty,
    cost,
  })
}

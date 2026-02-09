//
// Copyright (c) 2026-present, The Dash Core developers
// SPDX-License-Identifier: MIT
// See the accompanying file LICENSE or https://opensource.org/license/mit
//

use grovedb_version::version::GroveVersion;

use crate::element::{deserialize_element, serialize_element};
use crate::ffi::{FfiBoolResult, FfiChangedValueResult, FfiOperationCost, FfiOptionalElementResult};
use crate::lifecycle::operation_cost_to_ffi;
use crate::types::decode_path;
use crate::BoxedGroveDb;
use crate::BoxedTransaction;

// ---------------------------------------------------------------------------
// insert — unconditional insert
// ---------------------------------------------------------------------------

/// Insert an element at the given path and key, using default insert options.
pub fn grovedb_insert(
  db: &BoxedGroveDb,
  path: &[u8],
  key: &[u8],
  element: &[u8],
) -> Result<FfiOperationCost, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let elem = deserialize_element(element, version)?;
  let ctx = db
    .db
    .insert(segments.as_slice(), key, elem, None, None, version);
  let cost = operation_cost_to_ffi(&ctx.cost);
  ctx.value.map_err(crate::ffi_error)?;
  Ok(cost)
}

/// Insert an element at the given path and key within a transaction.
pub fn grovedb_insert_with_tx(
  db: &BoxedGroveDb,
  path: &[u8],
  key: &[u8],
  element: &[u8],
  tx: &BoxedTransaction,
) -> Result<FfiOperationCost, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let elem = deserialize_element(element, version)?;
  let ctx = db.db.insert(
    segments.as_slice(),
    key,
    elem,
    None,
    Some(&tx.tx),
    version,
  );
  let cost = operation_cost_to_ffi(&ctx.cost);
  ctx.value.map_err(crate::ffi_error)?;
  Ok(cost)
}

// ---------------------------------------------------------------------------
// insert_if_not_exists
// ---------------------------------------------------------------------------

/// Insert an element only if the key does not already exist.
///
/// Returns `value = true` if the element was inserted, `false` if the key
/// already existed (element is not modified in that case).
pub fn grovedb_insert_if_not_exists(
  db: &BoxedGroveDb,
  path: &[u8],
  key: &[u8],
  element: &[u8],
) -> Result<FfiBoolResult, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let elem = deserialize_element(element, version)?;
  let ctx = db
    .db
    .insert_if_not_exists(segments.as_slice(), key, elem, None, version);
  let cost = operation_cost_to_ffi(&ctx.cost);
  let inserted = ctx.value.map_err(crate::ffi_error)?;
  Ok(FfiBoolResult {
    value: inserted,
    cost,
  })
}

/// Insert an element only if the key does not already exist, within a transaction.
pub fn grovedb_insert_if_not_exists_with_tx(
  db: &BoxedGroveDb,
  path: &[u8],
  key: &[u8],
  element: &[u8],
  tx: &BoxedTransaction,
) -> Result<FfiBoolResult, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let elem = deserialize_element(element, version)?;
  let ctx = db.db.insert_if_not_exists(
    segments.as_slice(),
    key,
    elem,
    Some(&tx.tx),
    version,
  );
  let cost = operation_cost_to_ffi(&ctx.cost);
  let inserted = ctx.value.map_err(crate::ffi_error)?;
  Ok(FfiBoolResult {
    value: inserted,
    cost,
  })
}

// ---------------------------------------------------------------------------
// insert_if_not_exists_return_existing_element
// ---------------------------------------------------------------------------

/// Insert an element if the key does not exist; return the existing element
/// if it does.
///
/// Returns `has_element = false` when the new element was inserted (no
/// previous value), `has_element = true` with the serialized previous
/// element when the key already existed.
pub fn grovedb_insert_if_not_exists_return_existing(
  db: &BoxedGroveDb,
  path: &[u8],
  key: &[u8],
  element: &[u8],
) -> Result<FfiOptionalElementResult, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let elem = deserialize_element(element, version)?;
  let ctx = db.db.insert_if_not_exists_return_existing_element(
    segments.as_slice(),
    key,
    elem,
    None,
    version,
  );
  let cost = operation_cost_to_ffi(&ctx.cost);
  let maybe = ctx.value.map_err(crate::ffi_error)?;
  match maybe {
    Some(existing) => Ok(FfiOptionalElementResult {
      has_element: true,
      element: serialize_element(&existing, version)?,
      cost,
    }),
    None => Ok(FfiOptionalElementResult {
      has_element: false,
      element: Vec::new(),
      cost,
    }),
  }
}

/// Insert an element if the key does not exist, within a transaction.
pub fn grovedb_insert_if_not_exists_return_existing_with_tx(
  db: &BoxedGroveDb,
  path: &[u8],
  key: &[u8],
  element: &[u8],
  tx: &BoxedTransaction,
) -> Result<FfiOptionalElementResult, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let elem = deserialize_element(element, version)?;
  let ctx = db.db.insert_if_not_exists_return_existing_element(
    segments.as_slice(),
    key,
    elem,
    Some(&tx.tx),
    version,
  );
  let cost = operation_cost_to_ffi(&ctx.cost);
  let maybe = ctx.value.map_err(crate::ffi_error)?;
  match maybe {
    Some(existing) => Ok(FfiOptionalElementResult {
      has_element: true,
      element: serialize_element(&existing, version)?,
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
// insert_if_changed_value
// ---------------------------------------------------------------------------

/// Insert an element only if the value differs from the existing one.
///
/// Returns `changed = true` when a new value was written (either because
/// the key was absent or the value differed), with `has_previous_element`
/// indicating whether a prior value existed.
pub fn grovedb_insert_if_changed_value(
  db: &BoxedGroveDb,
  path: &[u8],
  key: &[u8],
  element: &[u8],
) -> Result<FfiChangedValueResult, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let elem = deserialize_element(element, version)?;
  let ctx = db
    .db
    .insert_if_changed_value(segments.as_slice(), key, elem, None, version);
  let cost = operation_cost_to_ffi(&ctx.cost);
  let (changed, maybe_prev) = ctx.value.map_err(crate::ffi_error)?;
  match maybe_prev {
    Some(prev) => Ok(FfiChangedValueResult {
      changed,
      has_previous_element: true,
      previous_element: serialize_element(&prev, version)?,
      cost,
    }),
    None => Ok(FfiChangedValueResult {
      changed,
      has_previous_element: false,
      previous_element: Vec::new(),
      cost,
    }),
  }
}

/// Insert an element only if the value differs, within a transaction.
pub fn grovedb_insert_if_changed_value_with_tx(
  db: &BoxedGroveDb,
  path: &[u8],
  key: &[u8],
  element: &[u8],
  tx: &BoxedTransaction,
) -> Result<FfiChangedValueResult, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let elem = deserialize_element(element, version)?;
  let ctx = db.db.insert_if_changed_value(
    segments.as_slice(),
    key,
    elem,
    Some(&tx.tx),
    version,
  );
  let cost = operation_cost_to_ffi(&ctx.cost);
  let (changed, maybe_prev) = ctx.value.map_err(crate::ffi_error)?;
  match maybe_prev {
    Some(prev) => Ok(FfiChangedValueResult {
      changed,
      has_previous_element: true,
      previous_element: serialize_element(&prev, version)?,
      cost,
    }),
    None => Ok(FfiChangedValueResult {
      changed,
      has_previous_element: false,
      previous_element: Vec::new(),
      cost,
    }),
  }
}

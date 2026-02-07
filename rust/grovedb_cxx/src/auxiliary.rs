//
// Copyright (c) 2026-present, The Dash Core developers
// SPDX-License-Identifier: MIT
// See the accompanying file LICENSE or https://opensource.org/license/mit
//

use grovedb_path::SubtreePath;
use grovedb_version::version::GroveVersion;

use crate::ffi::{FfiOperationCost, FfiOptionalBytesResult};
use crate::lifecycle::operation_cost_to_ffi;
use crate::types::decode_path;
use crate::BoxedGroveDb;
use crate::BoxedTransaction;

// ---------------------------------------------------------------------------
// put_aux
// ---------------------------------------------------------------------------

/// Store an auxiliary key-value pair.
pub fn grovedb_put_aux(
  db: &BoxedGroveDb,
  key: &[u8],
  value: &[u8],
) -> Result<FfiOperationCost, String> {
  let ctx = db.db.put_aux(key, value, None, None);
  let cost = operation_cost_to_ffi(&ctx.cost);
  ctx.value.map_err(|e| e.to_string())?;
  Ok(cost)
}

/// Store an auxiliary key-value pair within a transaction.
pub fn grovedb_put_aux_with_tx(
  db: &BoxedGroveDb,
  key: &[u8],
  value: &[u8],
  tx: &BoxedTransaction,
) -> Result<FfiOperationCost, String> {
  let ctx = db.db.put_aux(key, value, None, Some(&tx.tx));
  let cost = operation_cost_to_ffi(&ctx.cost);
  ctx.value.map_err(|e| e.to_string())?;
  Ok(cost)
}

// ---------------------------------------------------------------------------
// get_aux
// ---------------------------------------------------------------------------

/// Retrieve an auxiliary value by key.
///
/// Returns `has_value = false` if the key does not exist.
pub fn grovedb_get_aux(
  db: &BoxedGroveDb,
  key: &[u8],
) -> Result<FfiOptionalBytesResult, String> {
  let ctx = db.db.get_aux(key, None);
  let cost = operation_cost_to_ffi(&ctx.cost);
  let maybe = ctx.value.map_err(|e| e.to_string())?;
  match maybe {
    Some(v) => Ok(FfiOptionalBytesResult {
      has_value: true,
      value: v,
      cost,
    }),
    None => Ok(FfiOptionalBytesResult {
      has_value: false,
      value: Vec::new(),
      cost,
    }),
  }
}

/// Retrieve an auxiliary value by key within a transaction.
pub fn grovedb_get_aux_with_tx(
  db: &BoxedGroveDb,
  key: &[u8],
  tx: &BoxedTransaction,
) -> Result<FfiOptionalBytesResult, String> {
  let ctx = db.db.get_aux(key, Some(&tx.tx));
  let cost = operation_cost_to_ffi(&ctx.cost);
  let maybe = ctx.value.map_err(|e| e.to_string())?;
  match maybe {
    Some(v) => Ok(FfiOptionalBytesResult {
      has_value: true,
      value: v,
      cost,
    }),
    None => Ok(FfiOptionalBytesResult {
      has_value: false,
      value: Vec::new(),
      cost,
    }),
  }
}

// ---------------------------------------------------------------------------
// delete_aux
// ---------------------------------------------------------------------------

/// Delete an auxiliary key-value pair.
pub fn grovedb_delete_aux(
  db: &BoxedGroveDb,
  key: &[u8],
) -> Result<FfiOperationCost, String> {
  let ctx = db.db.delete_aux(key, None, None);
  let cost = operation_cost_to_ffi(&ctx.cost);
  ctx.value.map_err(|e| e.to_string())?;
  Ok(cost)
}

/// Delete an auxiliary key-value pair within a transaction.
pub fn grovedb_delete_aux_with_tx(
  db: &BoxedGroveDb,
  key: &[u8],
  tx: &BoxedTransaction,
) -> Result<FfiOperationCost, String> {
  let ctx = db.db.delete_aux(key, None, Some(&tx.tx));
  let cost = operation_cost_to_ffi(&ctx.cost);
  ctx.value.map_err(|e| e.to_string())?;
  Ok(cost)
}

// ---------------------------------------------------------------------------
// find_subtrees
// ---------------------------------------------------------------------------

/// Encode a `Vec<Vec<Vec<u8>>>` (list of paths) into wire format.
///
/// Wire format: `[u32 path_count][path₁][path₂]…`
/// Each path: `[u32 segment_count][u32 len₁][bytes₁]…`
fn encode_paths(paths: &[Vec<Vec<u8>>]) -> Vec<u8> {
  let mut buf = Vec::new();
  buf.extend_from_slice(&(paths.len() as u32).to_le_bytes());
  for path in paths {
    buf.extend_from_slice(&(path.len() as u32).to_le_bytes());
    for segment in path {
      buf.extend_from_slice(&(segment.len() as u32).to_le_bytes());
      buf.extend_from_slice(segment);
    }
  }
  buf
}

/// Find all subtree paths under the given root path.
///
/// Returns wire-encoded paths: `[u32 count][path₁][path₂]…`
pub fn grovedb_find_subtrees(
  db: &BoxedGroveDb,
  path: &[u8],
) -> Result<FfiOptionalBytesResult, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let subtree_path: SubtreePath<Vec<u8>> = segments.as_slice().into();
  let ctx = db.db.find_subtrees(&subtree_path, None, version);
  let cost = operation_cost_to_ffi(&ctx.cost);
  let paths = ctx.value.map_err(|e| e.to_string())?;
  Ok(FfiOptionalBytesResult {
    has_value: true,
    value: encode_paths(&paths),
    cost,
  })
}

/// Find all subtree paths under the given root path within a transaction.
pub fn grovedb_find_subtrees_with_tx(
  db: &BoxedGroveDb,
  path: &[u8],
  tx: &BoxedTransaction,
) -> Result<FfiOptionalBytesResult, String> {
  let version = GroveVersion::latest();
  let segments = decode_path(path)?;
  let subtree_path: SubtreePath<Vec<u8>> = segments.as_slice().into();
  let ctx = db
    .db
    .find_subtrees(&subtree_path, Some(&tx.tx), version);
  let cost = operation_cost_to_ffi(&ctx.cost);
  let paths = ctx.value.map_err(|e| e.to_string())?;
  Ok(FfiOptionalBytesResult {
    has_value: true,
    value: encode_paths(&paths),
    cost,
  })
}

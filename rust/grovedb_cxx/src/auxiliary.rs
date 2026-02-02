//
// Copyright (c) 2026-present, The Dash Core developers
// SPDX-License-Identifier: MIT
// See the accompanying file LICENSE or https://opensource.org/license/mit
//

use grovedb_path::SubtreePath;
use grovedb_version::version::GroveVersion;

use crate::ffi::{FfiFindSubtreesResult, FfiOperationCost, FfiOptionalBytesResult};
use crate::lifecycle::operation_cost_to_ffi;
use crate::types::decode_path;
use crate::BoxedGroveDb;
use crate::BoxedTransaction;

// ---------------------------------------------------------------------------
// put_aux — store auxiliary data
// ---------------------------------------------------------------------------

/// Store a key-value pair in auxiliary storage (outside the Merkle tree).
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

/// Store a key-value pair in auxiliary storage within a transaction.
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
// delete_aux — remove auxiliary data
// ---------------------------------------------------------------------------

/// Delete a key from auxiliary storage.
pub fn grovedb_delete_aux(
    db: &BoxedGroveDb,
    key: &[u8],
) -> Result<FfiOperationCost, String> {
    let ctx = db.db.delete_aux(key, None, None);
    let cost = operation_cost_to_ffi(&ctx.cost);
    ctx.value.map_err(|e| e.to_string())?;
    Ok(cost)
}

/// Delete a key from auxiliary storage within a transaction.
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
// get_aux — retrieve auxiliary data
// ---------------------------------------------------------------------------

/// Retrieve a value from auxiliary storage.
///
/// Returns `has_value = false` when the key is not found.
pub fn grovedb_get_aux(
    db: &BoxedGroveDb,
    key: &[u8],
) -> Result<FfiOptionalBytesResult, String> {
    let ctx = db.db.get_aux(key, None);
    let cost = operation_cost_to_ffi(&ctx.cost);
    let maybe = ctx.value.map_err(|e| e.to_string())?;
    match maybe {
        Some(value) => Ok(FfiOptionalBytesResult {
            has_value: true,
            value,
            cost,
        }),
        None => Ok(FfiOptionalBytesResult {
            has_value: false,
            value: Vec::new(),
            cost,
        }),
    }
}

/// Retrieve a value from auxiliary storage within a transaction.
pub fn grovedb_get_aux_with_tx(
    db: &BoxedGroveDb,
    key: &[u8],
    tx: &BoxedTransaction,
) -> Result<FfiOptionalBytesResult, String> {
    let ctx = db.db.get_aux(key, Some(&tx.tx));
    let cost = operation_cost_to_ffi(&ctx.cost);
    let maybe = ctx.value.map_err(|e| e.to_string())?;
    match maybe {
        Some(value) => Ok(FfiOptionalBytesResult {
            has_value: true,
            value,
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
// find_subtrees — discover all subtrees under a path
// ---------------------------------------------------------------------------

/// Encode a list of paths (Vec<Vec<Vec<u8>>>) into wire format.
///
/// Wire format:
/// ```text
/// [u32 path_count]
/// For each path:
///   [u32 segment_count]
///   For each segment:
///     [u32 len][bytes]
/// ```
fn encode_paths(paths: Vec<Vec<Vec<u8>>>) -> Vec<u8> {
    let mut buf = Vec::new();
    buf.extend_from_slice(&(paths.len() as u32).to_le_bytes());
    for path in &paths {
        buf.extend_from_slice(&(path.len() as u32).to_le_bytes());
        for seg in path {
            buf.extend_from_slice(&(seg.len() as u32).to_le_bytes());
            buf.extend_from_slice(seg);
        }
    }
    buf
}

/// Find all subtrees under a given path.
pub fn grovedb_find_subtrees(
    db: &BoxedGroveDb,
    path: &[u8],
) -> Result<FfiFindSubtreesResult, String> {
    let version = GroveVersion::latest();
    let segments = decode_path(path)?;
    let subtree_path: SubtreePath<Vec<u8>> = segments.as_slice().into();
    let ctx = db.db.find_subtrees(&subtree_path, None, version);
    let cost = operation_cost_to_ffi(&ctx.cost);
    let paths = ctx.value.map_err(|e| e.to_string())?;
    Ok(FfiFindSubtreesResult {
        paths: encode_paths(paths),
        cost,
    })
}

/// Find all subtrees under a given path within a transaction.
pub fn grovedb_find_subtrees_with_tx(
    db: &BoxedGroveDb,
    path: &[u8],
    tx: &BoxedTransaction,
) -> Result<FfiFindSubtreesResult, String> {
    let version = GroveVersion::latest();
    let segments = decode_path(path)?;
    let subtree_path: SubtreePath<Vec<u8>> = segments.as_slice().into();
    let ctx = db.db.find_subtrees(&subtree_path, Some(&tx.tx), version);
    let cost = operation_cost_to_ffi(&ctx.cost);
    let paths = ctx.value.map_err(|e| e.to_string())?;
    Ok(FfiFindSubtreesResult {
        paths: encode_paths(paths),
        cost,
    })
}

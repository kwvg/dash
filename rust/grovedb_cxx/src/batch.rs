//
// Copyright (c) 2026-present, The Dash Core developers
// SPDX-License-Identifier: MIT
// See the accompanying file LICENSE or https://opensource.org/license/mit
//

use grovedb::batch::{BatchApplyOptions, QualifiedGroveDbOp};
use grovedb::TreeType;
use grovedb_version::version::GroveVersion;

use crate::element::deserialize_element;
use crate::ffi::{FfiBatchApplyOptions, FfiOperationCost};
use crate::lifecycle::operation_cost_to_ffi;
use crate::types::decode_path;
use crate::BoxedGroveDb;
use crate::BoxedTransaction;

// ---------------------------------------------------------------------------
// Wire-format decoding
// ---------------------------------------------------------------------------

/// Op-kind discriminants matching the C++ `BatchOperation::Kind` enum.
const OP_INSERT_ONLY: u8 = 0;
const OP_INSERT_OR_REPLACE: u8 = 1;
const OP_REPLACE: u8 = 2;
const OP_DELETE: u8 = 3;
const OP_DELETE_TREE: u8 = 4;

/// Read a little-endian u32 from `data` at `offset`, advancing `offset`.
fn read_u32(data: &[u8], offset: &mut usize) -> Result<u32, String> {
    if *offset + 4 > data.len() {
        return Err(format!(
            "batch: unexpected end of data at offset {} (need 4 bytes, have {})",
            offset,
            data.len() - *offset
        ));
    }
    let val = u32::from_le_bytes(
        data[*offset..*offset + 4]
            .try_into()
            .map_err(|_| "batch: failed to read u32")?,
    );
    *offset += 4;
    Ok(val)
}

/// Read a single byte from `data` at `offset`, advancing `offset`.
fn read_u8(data: &[u8], offset: &mut usize) -> Result<u8, String> {
    if *offset >= data.len() {
        return Err(format!(
            "batch: unexpected end of data at offset {} (need 1 byte)",
            offset
        ));
    }
    let val = data[*offset];
    *offset += 1;
    Ok(val)
}

/// Read a length-prefixed byte slice from `data` at `offset`.
fn read_bytes(data: &[u8], offset: &mut usize) -> Result<Vec<u8>, String> {
    let len = read_u32(data, offset)? as usize;
    if *offset + len > data.len() {
        return Err(format!(
            "batch: unexpected end of data at offset {} (need {} bytes, have {})",
            offset,
            len,
            data.len() - *offset
        ));
    }
    let bytes = data[*offset..*offset + len].to_vec();
    *offset += len;
    Ok(bytes)
}

/// Read a wire-encoded path from `data` at `offset`.
///
/// Reuses the same format as `decode_path` but works inline with an offset
/// rather than consuming a separate slice.
fn read_path(data: &[u8], offset: &mut usize) -> Result<Vec<Vec<u8>>, String> {
    let seg_count = read_u32(data, offset)? as usize;
    let mut segments = Vec::with_capacity(seg_count);
    for _ in 0..seg_count {
        let seg = read_bytes(data, offset)?;
        segments.push(seg);
    }
    Ok(segments)
}

/// Decode wire-encoded batch operations from C++.
fn decode_batch_ops(data: &[u8]) -> Result<Vec<QualifiedGroveDbOp>, String> {
    let version = GroveVersion::latest();
    let mut offset = 0usize;

    let count = read_u32(data, &mut offset)? as usize;
    let mut ops = Vec::with_capacity(count);

    for i in 0..count {
        let path = read_path(data, &mut offset)?;
        let key = read_bytes(data, &mut offset)?;
        let kind = read_u8(data, &mut offset)?;

        let op = match kind {
            OP_INSERT_ONLY => {
                let elem_bytes = read_bytes(data, &mut offset)?;
                let elem = deserialize_element(&elem_bytes, version)?;
                QualifiedGroveDbOp::insert_only_op(path, key, elem)
            }
            OP_INSERT_OR_REPLACE => {
                let elem_bytes = read_bytes(data, &mut offset)?;
                let elem = deserialize_element(&elem_bytes, version)?;
                QualifiedGroveDbOp::insert_or_replace_op(path, key, elem)
            }
            OP_REPLACE => {
                let elem_bytes = read_bytes(data, &mut offset)?;
                let elem = deserialize_element(&elem_bytes, version)?;
                QualifiedGroveDbOp::replace_op(path, key, elem)
            }
            OP_DELETE => QualifiedGroveDbOp::delete_op(path, key),
            OP_DELETE_TREE => {
                let tree_type_byte = read_u8(data, &mut offset)?;
                let tree_type = TreeType::try_from(tree_type_byte).map_err(|_| {
                    format!("batch op {i}: invalid tree type {tree_type_byte}")
                })?;
                QualifiedGroveDbOp::delete_tree_op(path, key, tree_type)
            }
            _ => return Err(format!("batch op {i}: unknown op kind {kind}")),
        };

        ops.push(op);
    }

    Ok(ops)
}

/// Convert FFI options to Rust `BatchApplyOptions`.
fn convert_options(opts: &FfiBatchApplyOptions) -> BatchApplyOptions {
    BatchApplyOptions {
        validate_insertion_does_not_override: opts.validate_insertion_does_not_override,
        validate_insertion_does_not_override_tree: opts.validate_insertion_does_not_override_tree,
        allow_deleting_non_empty_trees: opts.allow_deleting_non_empty_trees,
        deleting_non_empty_trees_returns_error: opts.deleting_non_empty_trees_returns_error,
        disable_operation_consistency_check: opts.disable_operation_consistency_check,
        base_root_storage_is_free: opts.base_root_storage_is_free,
        batch_pause_height: None,
    }
}

// ---------------------------------------------------------------------------
// FFI bridge functions
// ---------------------------------------------------------------------------

/// Apply a batch of operations without a transaction.
pub(crate) fn grovedb_apply_batch(
    db: &BoxedGroveDb,
    ops: &[u8],
    options: &FfiBatchApplyOptions,
) -> Result<FfiOperationCost, String> {
    let ops_vec = decode_batch_ops(ops)?;
    let batch_options = convert_options(options);
    let version = GroveVersion::latest();
    let ctx = db
        .db
        .apply_batch(ops_vec, Some(batch_options), None, version);
    let cost = operation_cost_to_ffi(&ctx.cost);
    ctx.value.map_err(|e| e.to_string())?;
    Ok(cost)
}

/// Apply a batch of operations within a transaction.
pub(crate) fn grovedb_apply_batch_with_tx(
    db: &BoxedGroveDb,
    ops: &[u8],
    options: &FfiBatchApplyOptions,
    tx: &BoxedTransaction,
) -> Result<FfiOperationCost, String> {
    let ops_vec = decode_batch_ops(ops)?;
    let batch_options = convert_options(options);
    let version = GroveVersion::latest();
    let ctx =
        db.db
            .apply_batch(ops_vec, Some(batch_options), Some(&tx.tx), version);
    let cost = operation_cost_to_ffi(&ctx.cost);
    ctx.value.map_err(|e| e.to_string())?;
    Ok(cost)
}

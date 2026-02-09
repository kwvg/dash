//
// Copyright (c) 2026-present, The Dash Core developers
// SPDX-License-Identifier: MIT
// See the accompanying file LICENSE or https://opensource.org/license/mit
//

use binrw::binread;
use binrw::io::Cursor;
use binrw::BinRead;
use grovedb::batch::QualifiedGroveDbOp;
use grovedb::TreeType;
use grovedb_version::version::GroveVersion;

use crate::element::deserialize_element;
use crate::ffi::{FfiBatchApplyOptions, FfiOperationCost};
use crate::lifecycle::operation_cost_to_ffi;
use crate::BoxedGroveDb;
use crate::BoxedTransaction;

// ---------------------------------------------------------------------------
// Wire format types (deserialized via binrw)
// ---------------------------------------------------------------------------

/// Op discriminants matching the C++ `BatchOperation::Kind` enum.
const OP_INSERT_ONLY: u8 = 0;
const OP_INSERT_OR_REPLACE: u8 = 1;
const OP_REPLACE: u8 = 2;
const OP_DELETE: u8 = 3;
const OP_DELETE_TREE: u8 = 4;

/// A length-prefixed byte vector in the wire format.
///
/// Wire layout (little-endian): `[u32 len][bytes...]`
#[binread]
#[br(little)]
#[derive(Debug, Clone)]
struct WireBytes {
  #[br(temp)]
  len: u32,
  #[br(count = len)]
  data: Vec<u8>,
}

/// A wire-encoded path: counted sequence of byte-vector segments.
///
/// Wire layout (little-endian): `[u32 count][WireBytes₁][WireBytes₂]…`
#[binread]
#[br(little)]
#[derive(Debug, Clone)]
struct WirePath {
  #[br(temp)]
  count: u32,
  #[br(count = count)]
  segments: Vec<WireBytes>,
}

/// A single batch operation in wire format.
///
/// Wire layout:
/// ```text
/// [WirePath path]
/// [u32 key_len][key_bytes]
/// [u8 op_discriminant]
/// -- for insert ops (0,1,2): [u32 elem_len][elem_bytes]
/// -- for delete_tree (4): [u8 tree_type]
/// -- for delete (3): nothing
/// ```
#[binread]
#[br(little)]
#[derive(Debug)]
struct WireBatchOp {
  path: WirePath,
  key: WireBytes,
  op: u8,
  #[br(if(op <= 2))]
  element: Option<WireBytes>,
  #[br(if(op == 4))]
  tree_type: Option<u8>,
}

/// Wire-encoded list of batch operations.
///
/// Wire layout: `[u32 count][WireBatchOp₁][WireBatchOp₂]…`
#[binread]
#[br(little)]
#[derive(Debug)]
struct WireBatchOpList {
  #[br(temp)]
  count: u32,
  #[br(count = count)]
  ops: Vec<WireBatchOp>,
}

// ---------------------------------------------------------------------------
// Decoding
// ---------------------------------------------------------------------------

/// Decode a wire-encoded batch operation list into `QualifiedGroveDbOp` values.
fn decode_batch_ops(encoded: &[u8]) -> Result<Vec<QualifiedGroveDbOp>, String> {
  let mut cursor = Cursor::new(encoded);
  let list = WireBatchOpList::read_le(&mut cursor).map_err(crate::ffi_error_generic)?;
  let version = GroveVersion::latest();

  let mut ops = Vec::with_capacity(list.ops.len());
  for wire_op in list.ops {
    let path: Vec<Vec<u8>> = wire_op
      .path
      .segments
      .into_iter()
      .map(|s| s.data)
      .collect();
    let key = wire_op.key.data;

    let op = match wire_op.op {
      OP_INSERT_ONLY => {
        let elem_bytes = wire_op
          .element
          .ok_or_else(|| "INVALIDARG:missing element for InsertOnly".to_string())?;
        let elem = deserialize_element(&elem_bytes.data, version)?;
        QualifiedGroveDbOp::insert_only_op(path, key, elem)
      }
      OP_INSERT_OR_REPLACE => {
        let elem_bytes = wire_op
          .element
          .ok_or_else(|| "INVALIDARG:missing element for InsertOrReplace".to_string())?;
        let elem = deserialize_element(&elem_bytes.data, version)?;
        QualifiedGroveDbOp::insert_or_replace_op(path, key, elem)
      }
      OP_REPLACE => {
        let elem_bytes = wire_op
          .element
          .ok_or_else(|| "INVALIDARG:missing element for Replace".to_string())?;
        let elem = deserialize_element(&elem_bytes.data, version)?;
        QualifiedGroveDbOp::replace_op(path, key, elem)
      }
      OP_DELETE => QualifiedGroveDbOp::delete_op(path, key),
      OP_DELETE_TREE => {
        let tt = wire_op.tree_type.unwrap_or(0);
        let tree_type = TreeType::try_from(tt).map_err(crate::ffi_error_generic)?;
        QualifiedGroveDbOp::delete_tree_op(path, key, tree_type)
      }
      other => return Err(format!("INVALIDARG:unknown batch op discriminant: {other}")),
    };
    ops.push(op);
  }
  Ok(ops)
}

/// Convert `FfiBatchApplyOptions` into grovedb `BatchApplyOptions`.
fn convert_options(
  opts: &FfiBatchApplyOptions,
) -> grovedb::batch::BatchApplyOptions {
  grovedb::batch::BatchApplyOptions {
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
// FFI functions
// ---------------------------------------------------------------------------

/// Apply a batch of operations atomically.
///
/// The `encoded_ops` parameter contains the wire-encoded batch operation list.
/// See `WireBatchOpList` for the wire format.
pub fn grovedb_apply_batch(
  db: &BoxedGroveDb,
  encoded_ops: &[u8],
  options: &FfiBatchApplyOptions,
) -> Result<FfiOperationCost, String> {
  let version = GroveVersion::latest();
  let ops = decode_batch_ops(encoded_ops)?;
  let opts = convert_options(options);
  let ctx = db.db.apply_batch(ops, Some(opts), None, version);
  let cost = operation_cost_to_ffi(&ctx.cost);
  ctx.value.map_err(crate::ffi_error)?;
  Ok(cost)
}

/// Apply a batch of operations atomically within a transaction.
pub fn grovedb_apply_batch_with_tx(
  db: &BoxedGroveDb,
  encoded_ops: &[u8],
  options: &FfiBatchApplyOptions,
  tx: &BoxedTransaction,
) -> Result<FfiOperationCost, String> {
  let version = GroveVersion::latest();
  let ops = decode_batch_ops(encoded_ops)?;
  let opts = convert_options(options);
  let ctx = db
    .db
    .apply_batch(ops, Some(opts), Some(&tx.tx), version);
  let cost = operation_cost_to_ffi(&ctx.cost);
  ctx.value.map_err(crate::ffi_error)?;
  Ok(cost)
}

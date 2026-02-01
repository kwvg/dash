//
// Copyright (c) 2026-present, The Dash Core developers
// SPDX-License-Identifier: MIT
// See the accompanying file LICENSE or https://opensource.org/license/mit
//

use grovedb::operations::proof::ProveOptions;
use grovedb::query_result_type::PathKeyOptionalElementTrio;
use grovedb::GroveDb;
use grovedb::VerifyOptions;
use grovedb_version::version::GroveVersion;

use crate::element::serialize_element;
use crate::ffi::{FfiProofResult, FfiVerifyResult};
use crate::lifecycle::operation_cost_to_ffi;
use crate::BoxedGroveDb;
use crate::BoxedPathQuery;

// ---------------------------------------------------------------------------
// Wire encoding helpers
// ---------------------------------------------------------------------------

/// Encode a path (Vec<Vec<u8>>) into the wire format used by the C++ side.
///
/// Wire format: `[u32 seg_count][u32 len₁][bytes₁][u32 len₂][bytes₂]…`
fn encode_path(path: &[Vec<u8>], buf: &mut Vec<u8>) {
    buf.extend_from_slice(&(path.len() as u32).to_le_bytes());
    for seg in path {
        buf.extend_from_slice(&(seg.len() as u32).to_le_bytes());
        buf.extend_from_slice(seg);
    }
}

/// Encode verification result entries into the wire format.
///
/// Wire format:
/// ```text
/// [u32 count]
/// for each entry:
///   [path]                           — wire-encoded path
///   [u32 key_len][key bytes]         — length-prefixed key
///   [u8 has_element]                 — 0 or 1
///   if has_element:
///     [u32 elem_len][elem bytes]     — bincode-serialized Element
/// ```
fn encode_verify_entries(
    entries: Vec<PathKeyOptionalElementTrio>,
) -> Result<Vec<u8>, String> {
    let version = GroveVersion::latest();
    let mut buf = Vec::new();
    buf.extend_from_slice(&(entries.len() as u32).to_le_bytes());

    for (path, key, opt_element) in entries {
        // path
        encode_path(&path, &mut buf);
        // key
        buf.extend_from_slice(&(key.len() as u32).to_le_bytes());
        buf.extend_from_slice(&key);
        // has_element + optional element
        match opt_element {
            Some(element) => {
                buf.push(1u8);
                let elem_bytes = serialize_element(&element, version)?;
                buf.extend_from_slice(&(elem_bytes.len() as u32).to_le_bytes());
                buf.extend_from_slice(&elem_bytes);
            }
            None => {
                buf.push(0u8);
            }
        }
    }

    Ok(buf)
}

/// Convert a verification result (hash + entries) into the FFI result struct.
fn make_verify_result(
    hash: [u8; 32],
    entries: Vec<PathKeyOptionalElementTrio>,
) -> Result<FfiVerifyResult, String> {
    let entries_buf = encode_verify_entries(entries)?;
    Ok(FfiVerifyResult {
        root_hash: hash.to_vec(),
        entries: entries_buf,
    })
}

// ---------------------------------------------------------------------------
// Proof generation
// ---------------------------------------------------------------------------

/// Generate a cryptographic proof for a path query.
pub(crate) fn grovedb_prove_query(
    db: &BoxedGroveDb,
    query: &BoxedPathQuery,
    decrease_limit_on_empty: bool,
) -> Result<FfiProofResult, String> {
    let version = GroveVersion::latest();
    let options = ProveOptions {
        decrease_limit_on_empty_sub_query_result: decrease_limit_on_empty,
    };

    let ctx = db
        .db
        .prove_query(&query.query, Some(options), version);
    let cost = operation_cost_to_ffi(&ctx.cost);
    let proof = ctx.value.map_err(|e| e.to_string())?;

    Ok(FfiProofResult { proof, cost })
}

// ---------------------------------------------------------------------------
// Proof verification
// ---------------------------------------------------------------------------

/// Verify a proof against a query using default options.
pub(crate) fn grovedb_verify_query(
    proof: &[u8],
    query: &BoxedPathQuery,
) -> Result<FfiVerifyResult, String> {
    let version = GroveVersion::latest();
    let (hash, entries) = GroveDb::verify_query(proof, &query.query, version)
        .map_err(|e| e.to_string())?;
    make_verify_result(hash, entries)
}

/// Verify a proof against a query with explicit options.
pub(crate) fn grovedb_verify_query_with_options(
    proof: &[u8],
    query: &BoxedPathQuery,
    absence_proofs: bool,
    verify_succinctness: bool,
    include_empty_trees: bool,
) -> Result<FfiVerifyResult, String> {
    let version = GroveVersion::latest();
    let options = VerifyOptions {
        absence_proofs_for_non_existing_searched_keys: absence_proofs,
        verify_proof_succinctness: verify_succinctness,
        include_empty_trees_in_result: include_empty_trees,
    };
    let (hash, entries) =
        GroveDb::verify_query_with_options(proof, &query.query, options, version)
            .map_err(|e| e.to_string())?;
    make_verify_result(hash, entries)
}

/// Verify a subset proof against a query.
pub(crate) fn grovedb_verify_subset_query(
    proof: &[u8],
    query: &BoxedPathQuery,
) -> Result<FfiVerifyResult, String> {
    let version = GroveVersion::latest();
    let (hash, entries) = GroveDb::verify_subset_query(proof, &query.query, version)
        .map_err(|e| e.to_string())?;
    make_verify_result(hash, entries)
}

/// Verify a proof and check for absence proofs of non-existing keys.
pub(crate) fn grovedb_verify_query_with_absence_proof(
    proof: &[u8],
    query: &BoxedPathQuery,
) -> Result<FfiVerifyResult, String> {
    let version = GroveVersion::latest();
    let (hash, entries) =
        GroveDb::verify_query_with_absence_proof(proof, &query.query, version)
            .map_err(|e| e.to_string())?;
    make_verify_result(hash, entries)
}

/// Verify a subset proof with absence proofs.
pub(crate) fn grovedb_verify_subset_query_with_absence_proof(
    proof: &[u8],
    query: &BoxedPathQuery,
) -> Result<FfiVerifyResult, String> {
    let version = GroveVersion::latest();
    let (hash, entries) =
        GroveDb::verify_subset_query_with_absence_proof(proof, &query.query, version)
            .map_err(|e| e.to_string())?;
    make_verify_result(hash, entries)
}

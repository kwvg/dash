//
// Copyright (c) 2026-present, The Dash Core developers
// SPDX-License-Identifier: MIT
// See the accompanying file LICENSE or https://opensource.org/license/mit
//

use grovedb::operations::proof::ProveOptions;
use grovedb::{GroveDb, VerifyOptions};
use grovedb_version::version::GroveVersion;

use crate::ffi::{FfiChainedVerifyResult, FfiProofResult, FfiVerifyResult};
use crate::lifecycle::operation_cost_to_ffi;
use crate::query::encode_path_key_element_triples;
use crate::BoxedGroveDb;
use crate::BoxedPathQuery;
use crate::BoxedPathQueryVec;

// ---------------------------------------------------------------------------
// Prove
// ---------------------------------------------------------------------------

/// Generate a proof for a path query.
///
/// `decrease_limit_on_empty` controls the `ProveOptions` flag
/// `decrease_limit_on_empty_sub_query_result` (default `true`).
pub fn grovedb_prove_query(
  db: &BoxedGroveDb,
  query: &BoxedPathQuery,
  decrease_limit_on_empty: bool,
) -> Result<FfiProofResult, String> {
  let version = GroveVersion::latest();
  let opts = ProveOptions {
    decrease_limit_on_empty_sub_query_result: decrease_limit_on_empty,
  };

  let cost_result = db.db.prove_query(&query.query, Some(opts), version);
  let cost = operation_cost_to_ffi(&cost_result.cost);
  let proof = cost_result.value.map_err(|e| e.to_string())?;

  Ok(FfiProofResult { proof, cost })
}

// ---------------------------------------------------------------------------
// Verify helpers
// ---------------------------------------------------------------------------

/// Zero-valued cost for pure verification (no DB mutation).
fn zero_cost() -> crate::ffi::FfiOperationCost {
  crate::ffi::FfiOperationCost {
    seek_count: 0,
    storage_added_bytes: 0,
    storage_replaced_bytes: 0,
    storage_removed_bytes: 0,
    storage_loaded_bytes: 0,
    hash_node_calls: 0,
  }
}

/// Common verify implementation parameterised by `VerifyOptions`.
fn verify_impl(
  proof: &[u8],
  query: &BoxedPathQuery,
  options: VerifyOptions,
) -> Result<FfiVerifyResult, String> {
  let version = GroveVersion::latest();
  let (root_hash, results) = GroveDb::verify_query_with_options(proof, &query.query, options, version)
    .map_err(|e| e.to_string())?;

  let encoded = encode_path_key_element_triples(results)?;

  Ok(FfiVerifyResult {
    root_hash: root_hash.to_vec(),
    results: encoded,
    cost: zero_cost(),
  })
}

// ---------------------------------------------------------------------------
// Verify variants
// ---------------------------------------------------------------------------

/// Verify a query proof with strict succinctness.
pub fn grovedb_verify_query(
  proof: &[u8],
  query: &BoxedPathQuery,
) -> Result<FfiVerifyResult, String> {
  verify_impl(
    proof,
    query,
    VerifyOptions {
      absence_proofs_for_non_existing_searched_keys: false,
      verify_proof_succinctness: true,
      include_empty_trees_in_result: false,
    },
  )
}

/// Verify a query proof with custom options.
///
/// The three booleans map to the `VerifyOptions` fields:
/// `absence_proofs`, `verify_succinctness`, `include_empty_trees`.
pub fn grovedb_verify_query_with_options(
  proof: &[u8],
  query: &BoxedPathQuery,
  absence_proofs: bool,
  verify_succinctness: bool,
  include_empty_trees: bool,
) -> Result<FfiVerifyResult, String> {
  verify_impl(
    proof,
    query,
    VerifyOptions {
      absence_proofs_for_non_existing_searched_keys: absence_proofs,
      verify_proof_succinctness: verify_succinctness,
      include_empty_trees_in_result: include_empty_trees,
    },
  )
}

/// Verify a subset query proof (non-strict succinctness).
pub fn grovedb_verify_subset_query(
  proof: &[u8],
  query: &BoxedPathQuery,
) -> Result<FfiVerifyResult, String> {
  verify_impl(
    proof,
    query,
    VerifyOptions {
      absence_proofs_for_non_existing_searched_keys: false,
      verify_proof_succinctness: false,
      include_empty_trees_in_result: false,
    },
  )
}

/// Verify a query proof with absence proofs for non-existing keys.
pub fn grovedb_verify_query_with_absence_proof(
  proof: &[u8],
  query: &BoxedPathQuery,
) -> Result<FfiVerifyResult, String> {
  verify_impl(
    proof,
    query,
    VerifyOptions {
      absence_proofs_for_non_existing_searched_keys: true,
      verify_proof_succinctness: true,
      include_empty_trees_in_result: false,
    },
  )
}

/// Verify a subset query proof with absence proofs.
pub fn grovedb_verify_subset_query_with_absence_proof(
  proof: &[u8],
  query: &BoxedPathQuery,
) -> Result<FfiVerifyResult, String> {
  verify_impl(
    proof,
    query,
    VerifyOptions {
      absence_proofs_for_non_existing_searched_keys: true,
      verify_proof_succinctness: false,
      include_empty_trees_in_result: false,
    },
  )
}

// ---------------------------------------------------------------------------
// Chained query verification
// ---------------------------------------------------------------------------

/// Verify a proof against a sequence of chained queries.
///
/// The first query is verified against the proof.  Then, for each subsequent
/// query in `chained_queries`, a new `PathQuery` is constructed by appending
/// the first result key of the previous query to the chained query's path,
/// and the same proof is verified again.
///
/// Returns the root hash and the results of all queries encoded sequentially.
///
/// Wire format of `results`:
///   `[u32 query_count]`
///   For each query: `[u32 entry_count][PathKeyOptionalElementTrio entries…]`
pub fn grovedb_verify_chained_queries(
  proof: &[u8],
  first_query: &BoxedPathQuery,
  chained_queries: &BoxedPathQueryVec,
) -> Result<FfiChainedVerifyResult, String> {
  let version = GroveVersion::latest();

  let verify_options = VerifyOptions {
    absence_proofs_for_non_existing_searched_keys: false,
    verify_proof_succinctness: true,
    include_empty_trees_in_result: false,
  };

  // Verify the first query.
  let (root_hash, first_results) =
    GroveDb::verify_query_with_options(proof, &first_query.query, verify_options, version)
      .map_err(|e| e.to_string())?;

  let mut all_results: Vec<Vec<u8>> = Vec::with_capacity(1 + chained_queries.queries.len());
  all_results.push(encode_path_key_element_triples(first_results.clone())?);

  // Keep track of the "last first key" for chaining.
  let mut last_first_key: Option<Vec<u8>> = first_results
    .first()
    .map(|(_, key, _)| key.clone());

  // Process chained queries.
  for chained_pq in &chained_queries.queries {
    let chained_pq = match &last_first_key {
      Some(key) => {
        // Append the key from the previous result to the chained query's path.
        let mut new_path = chained_pq.path.clone();
        new_path.push(key.clone());
        grovedb::PathQuery::new(
          new_path,
          chained_pq.query.clone(),
        )
      }
      None => chained_pq.clone(),
    };

    let (_, results) =
      GroveDb::verify_query_with_options(proof, &chained_pq, verify_options, version)
        .map_err(|e| e.to_string())?;

    // Update the key for the next chain link.
    last_first_key = results.first().map(|(_, key, _)| key.clone());

    all_results.push(encode_path_key_element_triples(results)?);
  }

  // Encode all results: [u32 query_count][results₁][results₂]…
  let query_count = all_results.len();
  let total_size: usize = 4 + all_results.iter().map(|r| r.len()).sum::<usize>();
  let mut buf = Vec::with_capacity(total_size);
  buf.extend_from_slice(
    &u32::try_from(query_count)
      .map_err(|_| "too many chained queries".to_string())?
      .to_le_bytes(),
  );
  for result_bytes in &all_results {
    buf.extend_from_slice(result_bytes);
  }

  Ok(FfiChainedVerifyResult {
    root_hash: root_hash.to_vec(),
    results: buf,
    cost: zero_cost(),
  })
}

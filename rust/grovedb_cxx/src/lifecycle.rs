//
// Copyright (c) 2026-present, The Dash Core developers
// SPDX-License-Identifier: MIT
// See the accompanying file LICENSE or https://opensource.org/license/mit
//

use std::path::Path;

use grovedb::GroveDb;
use grovedb_costs::storage_cost::removal::StorageRemovedBytes;
use grovedb_version::version::GroveVersion;

use crate::ffi::{FfiOperationCost, FfiRootHashResult};

/// Opaque wrapper around `grovedb::GroveDb` for use across the CXX bridge.
pub struct BoxedGroveDb {
  pub(crate) db: grovedb::GroveDb,
}

impl std::fmt::Debug for BoxedGroveDb {
  fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
    f.debug_struct("BoxedGroveDb").finish_non_exhaustive()
  }
}

/// Convert a `grovedb_costs::OperationCost` into the flat FFI-safe struct.
///
/// `StorageRemovedBytes::SectionedStorageRemoval` is collapsed to a single
/// `u32` total — the per-identity-per-epoch breakdown is only meaningful for
/// the callback-based methods which are excluded from the bridge.
pub(crate) fn operation_cost_to_ffi(cost: &grovedb_costs::OperationCost) -> FfiOperationCost {
  let removed_bytes = match &cost.storage_cost.removed_bytes {
    StorageRemovedBytes::NoStorageRemoval => 0,
    StorageRemovedBytes::BasicStorageRemoval(n) => *n,
    StorageRemovedBytes::SectionedStorageRemoval(map) => map
      .values()
      .flat_map(|epoch_map| epoch_map.iter().map(|(_, v)| *v))
      .fold(0u32, |acc, v| acc.saturating_add(v)),
  };

  FfiOperationCost {
    seek_count: cost.seek_count,
    storage_added_bytes: cost.storage_cost.added_bytes,
    storage_replaced_bytes: cost.storage_cost.replaced_bytes,
    storage_removed_bytes: removed_bytes,
    storage_loaded_bytes: cost.storage_loaded_bytes,
    hash_node_calls: cost.hash_node_calls,
  }
}

/// Open or create a GroveDB instance at the given filesystem path.
pub fn grovedb_open(path: &str) -> Result<Box<BoxedGroveDb>, String> {
  let db = GroveDb::open(Path::new(path)).map_err(crate::ffi_error)?;
  Ok(Box::new(BoxedGroveDb { db }))
}

/// Flush the in-memory write buffer to persistent storage.
pub fn grovedb_flush(db: &BoxedGroveDb) -> Result<(), String> {
  db.db.flush().map_err(crate::ffi_error)
}

/// Delete all GroveDB key-value pairs from the underlying storage.
pub fn grovedb_wipe(db: &BoxedGroveDb) -> Result<(), String> {
  db.db.wipe().map_err(crate::ffi_error)
}

/// Return the 32-byte Merkle root hash together with operation costs.
pub fn grovedb_root_hash(db: &BoxedGroveDb) -> Result<FfiRootHashResult, String> {
  let version = GroveVersion::latest();
  let cost_result = db.db.root_hash(None, version);
  let cost = operation_cost_to_ffi(&cost_result.cost);
  let hash = cost_result.value.map_err(crate::ffi_error)?;

  Ok(FfiRootHashResult {
    root_hash: hash.to_vec(),
    cost,
  })
}

/// Verify the cryptographic integrity of the entire tree.
///
/// Returns `true` when no issues are found (all value hashes match).
pub fn grovedb_verify(db: &BoxedGroveDb) -> Result<bool, String> {
  let version = GroveVersion::latest();
  let issues = db
    .db
    .verify_grovedb(None, true, false, version)
    .map_err(crate::ffi_error)?;
  Ok(issues.is_empty())
}

/// Return crate name, version, and compiler used to build it.
pub fn whoami() -> String {
  format!(
    "{} {} built with {}",
    crate::built_info::PKG_NAME,
    crate::built_info::PKG_VERSION,
    crate::built_info::RUSTC_VERSION
  )
}

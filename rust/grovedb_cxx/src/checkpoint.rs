//
// Copyright (c) 2026-present, The Dash Core developers
// SPDX-License-Identifier: MIT
// See the accompanying file LICENSE or https://opensource.org/license/mit
//

use grovedb::GroveDb;

use crate::BoxedGroveDb;

// ---------------------------------------------------------------------------
// Checkpoint operations
// ---------------------------------------------------------------------------

/// Create a checkpoint of the database at the given filesystem path.
pub fn grovedb_create_checkpoint(db: &BoxedGroveDb, path: &str) -> Result<(), String> {
  db.db.create_checkpoint(path).map_err(crate::ffi_error)
}

/// Open an existing checkpoint as a read-only database handle.
pub fn grovedb_open_checkpoint(path: &str) -> Result<Box<BoxedGroveDb>, String> {
  let db = GroveDb::open_checkpoint(path).map_err(crate::ffi_error)?;
  Ok(Box::new(BoxedGroveDb { db }))
}

/// Delete a checkpoint directory.
///
/// Verifies the path is a valid checkpoint before deletion.
pub fn grovedb_delete_checkpoint(path: &str) -> Result<(), String> {
  GroveDb::delete_checkpoint(path).map_err(crate::ffi_error)
}

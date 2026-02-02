//
// Copyright (c) 2026-present, The Dash Core developers
// SPDX-License-Identifier: MIT
// See the accompanying file LICENSE or https://opensource.org/license/mit
//

use grovedb::GroveDb;

use crate::BoxedGroveDb;

// ---------------------------------------------------------------------------
// create_checkpoint — snapshot current DB state
// ---------------------------------------------------------------------------

/// Create a filesystem checkpoint of the current database state.
pub fn grovedb_create_checkpoint(db: &BoxedGroveDb, path: &str) -> Result<(), String> {
    db.db.create_checkpoint(path).map_err(|e| e.to_string())
}

// ---------------------------------------------------------------------------
// open_checkpoint — open a DB instance from a checkpoint
// ---------------------------------------------------------------------------

/// Open a GroveDB instance from a previously created checkpoint.
pub fn grovedb_open_checkpoint(path: &str) -> Result<Box<BoxedGroveDb>, String> {
    let db = GroveDb::open_checkpoint(path).map_err(|e| e.to_string())?;
    Ok(Box::new(BoxedGroveDb { db }))
}

// ---------------------------------------------------------------------------
// delete_checkpoint — safely remove a checkpoint directory
// ---------------------------------------------------------------------------

/// Delete a checkpoint directory after verifying it is a valid GroveDB checkpoint.
pub fn grovedb_delete_checkpoint(path: &str) -> Result<(), String> {
    GroveDb::delete_checkpoint(path).map_err(|e| e.to_string())
}

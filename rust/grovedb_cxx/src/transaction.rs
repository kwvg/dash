//
// Copyright (c) 2026-present, The Dash Core developers
// SPDX-License-Identifier: MIT
// See the accompanying file LICENSE or https://opensource.org/license/mit
//

use crate::ffi::FfiOperationCost;
use crate::lifecycle::operation_cost_to_ffi;
use crate::BoxedGroveDb;
use crate::BoxedTransaction;

/// Start a new database transaction.
///
/// # Safety
///
/// The returned [`BoxedTransaction`] erases the lifetime of the underlying
/// `Transaction<'db>`.  The C++ `Transaction` RAII wrapper **must** ensure
/// the `BoxedTransaction` does not outlive the `BoxedGroveDb` that created it.
#[allow(unsafe_code)]
pub fn grovedb_start_transaction(db: &BoxedGroveDb) -> Result<Box<BoxedTransaction>, String> {
    let tx = db.db.start_transaction();
    // Safety: C++ Transaction RAII wrapper ensures this does not outlive GroveDb.
    let tx: grovedb::Transaction<'static> = unsafe { std::mem::transmute(tx) };
    Ok(Box::new(BoxedTransaction { tx }))
}

/// Commit a previously started transaction.
///
/// The transaction is consumed — it cannot be used after this call.
#[allow(unsafe_code)]
pub fn grovedb_commit_transaction(
    db: &BoxedGroveDb,
    tx: Box<BoxedTransaction>,
) -> Result<FfiOperationCost, String> {
    let inner = *tx;
    // Safety: transmute back from 'static to the true lifetime tied to `db`.
    let real_tx: grovedb::Transaction<'_> = unsafe { std::mem::transmute(inner.tx) };
    let cost_result = db.db.commit_transaction(real_tx);
    let cost = operation_cost_to_ffi(&cost_result.cost);
    cost_result.value.map_err(|e| e.to_string())?;
    Ok(cost)
}

/// Roll back a previously started transaction without consuming it.
///
/// The transaction remains valid after rollback (it is reset to its initial
/// state) and will be destroyed when the owning `Box` is dropped.
pub fn grovedb_rollback_transaction(
    db: &BoxedGroveDb,
    tx: &BoxedTransaction,
) -> Result<(), String> {
    db.db
        .rollback_transaction(&tx.tx)
        .map_err(|e| e.to_string())
}

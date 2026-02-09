//
// Copyright (c) 2026-present, The Dash Core developers
// SPDX-License-Identifier: MIT
// See the accompanying file LICENSE or https://opensource.org/license/mit
//

use crate::ffi::FfiOperationCost;
use crate::lifecycle::operation_cost_to_ffi;
use crate::BoxedGroveDb;

/// Opaque wrapper around a `grovedb::Transaction` with an erased lifetime.
///
/// # Safety
///
/// The `'static` lifetime is a lie — the C++ `Transaction` RAII wrapper
/// enforces the invariant that this never outlives the `BoxedGroveDb` that
/// created it.
#[allow(unsafe_code)]
pub struct BoxedTransaction {
  pub(crate) tx: grovedb::Transaction<'static>,
}

impl std::fmt::Debug for BoxedTransaction {
  fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
    f.debug_struct("BoxedTransaction").finish_non_exhaustive()
  }
}

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
  cost_result.value.map_err(crate::ffi_error)?;
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
    .map_err(crate::ffi_error)
}

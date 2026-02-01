//
// Copyright (c) 2026-present, The Dash Core developers
// SPDX-License-Identifier: MIT
// See the accompanying file LICENSE or https://opensource.org/license/mit
//

pub use grovedb;

mod built_info {
  include!(concat!(env!("OUT_DIR"), "/built.rs"));
}

mod lifecycle;
use lifecycle::{grovedb_flush, grovedb_open, grovedb_root_hash, grovedb_verify, grovedb_wipe};

mod transaction;
use transaction::{grovedb_commit_transaction, grovedb_rollback_transaction, grovedb_start_transaction};

/// Opaque wrapper around `grovedb::GroveDb` for use across the CXX bridge.
pub struct BoxedGroveDb {
  db: grovedb::GroveDb,
}

impl std::fmt::Debug for BoxedGroveDb {
  fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
    f.debug_struct("BoxedGroveDb").finish_non_exhaustive()
  }
}

/// Opaque wrapper around a `grovedb::Transaction` with an erased lifetime.
///
/// # Safety
///
/// The `'static` lifetime is a lie — the C++ `Transaction` RAII wrapper
/// enforces the invariant that this never outlives the `BoxedGroveDb` that
/// created it.
#[allow(unsafe_code)]
pub struct BoxedTransaction {
  tx: grovedb::Transaction<'static>,
}

impl std::fmt::Debug for BoxedTransaction {
  fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
    f.debug_struct("BoxedTransaction").finish_non_exhaustive()
  }
}

#[allow(unsafe_code)]
#[cxx::bridge(namespace = "grovedb_cxx")]
pub(crate) mod ffi {
  /// Operation cost returned alongside every cost-tracked result.
  struct FfiOperationCost {
    seek_count: u32,
    storage_added_bytes: u32,
    storage_replaced_bytes: u32,
    storage_removed_bytes: u32,
    storage_loaded_bytes: u64,
    hash_node_calls: u32,
  }

  /// Result of `root_hash`: the 32-byte Merkle root plus operation costs.
  struct FfiRootHashResult {
    root_hash: Vec<u8>,
    cost: FfiOperationCost,
  }

  extern "Rust" {
    type BoxedGroveDb;
    type BoxedTransaction;

    fn whoami() -> String;

    // -- Lifecycle --
    fn grovedb_open(path: &str) -> Result<Box<BoxedGroveDb>>;
    fn grovedb_flush(db: &BoxedGroveDb) -> Result<()>;
    fn grovedb_wipe(db: &BoxedGroveDb) -> Result<()>;
    fn grovedb_root_hash(db: &BoxedGroveDb) -> Result<FfiRootHashResult>;
    fn grovedb_verify(db: &BoxedGroveDb) -> Result<bool>;

    // -- Transactions --
    fn grovedb_start_transaction(db: &BoxedGroveDb) -> Result<Box<BoxedTransaction>>;
    fn grovedb_commit_transaction(db: &BoxedGroveDb, tx: Box<BoxedTransaction>) -> Result<FfiOperationCost>;
    fn grovedb_rollback_transaction(db: &BoxedGroveDb, tx: &BoxedTransaction) -> Result<()>;
  }
}

fn whoami() -> String {
  format!("{} {} built with {}", built_info::PKG_NAME, built_info::PKG_VERSION, built_info::RUSTC_VERSION)
}

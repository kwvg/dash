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

/// Opaque wrapper around `grovedb::GroveDb` for use across the CXX bridge.
pub struct BoxedGroveDb {
  db: grovedb::GroveDb,
}

impl std::fmt::Debug for BoxedGroveDb {
  fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
    f.debug_struct("BoxedGroveDb").finish_non_exhaustive()
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

    fn whoami() -> String;

    // -- Lifecycle --
    fn grovedb_open(path: &str) -> Result<Box<BoxedGroveDb>>;
    fn grovedb_flush(db: &BoxedGroveDb) -> Result<()>;
    fn grovedb_wipe(db: &BoxedGroveDb) -> Result<()>;
    fn grovedb_root_hash(db: &BoxedGroveDb) -> Result<FfiRootHashResult>;
    fn grovedb_verify(db: &BoxedGroveDb) -> Result<bool>;
  }
}

fn whoami() -> String {
  format!("{} {} built with {}", built_info::PKG_NAME, built_info::PKG_VERSION, built_info::RUSTC_VERSION)
}

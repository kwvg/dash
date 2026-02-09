//
// Copyright (c) 2026-present, The Dash Core developers
// SPDX-License-Identifier: MIT
// See the accompanying file LICENSE or https://opensource.org/license/mit
//

use grovedb_version::version::GroveVersion;

/// Serialize an Element to its bincode wire format.
pub(crate) fn serialize_element(
  element: &grovedb::Element,
  version: &GroveVersion,
) -> Result<Vec<u8>, String> {
  element
    .serialize(version)
    .map_err(|e| crate::ffi_error(grovedb::Error::from(e)))
}

/// Deserialize an Element from its bincode wire format.
pub(crate) fn deserialize_element(
  bytes: &[u8],
  version: &GroveVersion,
) -> Result<grovedb::Element, String> {
  grovedb::Element::deserialize(bytes, version)
    .map_err(|e| crate::ffi_error(grovedb::Error::from(e)))
}

// ---------------------------------------------------------------------------
// Element factory bridge functions
// ---------------------------------------------------------------------------

/// Create a serialized `Element::new_item` from raw value bytes.
pub fn grovedb_element_item(value: &[u8]) -> Result<Vec<u8>, String> {
  let version = GroveVersion::latest();
  let element = grovedb::Element::new_item(value.to_vec());
  serialize_element(&element, version)
}

/// Create a serialized `Element::empty_tree`.
pub fn grovedb_element_empty_tree() -> Result<Vec<u8>, String> {
  let version = GroveVersion::latest();
  let element = grovedb::Element::empty_tree();
  serialize_element(&element, version)
}

/// Create a serialized `Element::empty_sum_tree`.
pub fn grovedb_element_empty_sum_tree() -> Result<Vec<u8>, String> {
  let version = GroveVersion::latest();
  let element = grovedb::Element::empty_sum_tree();
  serialize_element(&element, version)
}

/// Create a serialized `Element::new_sum_item(value)`.
pub fn grovedb_element_sum_item(value: i64) -> Result<Vec<u8>, String> {
  let version = GroveVersion::latest();
  let element = grovedb::Element::new_sum_item(value);
  serialize_element(&element, version)
}

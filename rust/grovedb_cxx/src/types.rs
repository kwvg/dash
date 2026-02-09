//
// Copyright (c) 2026-present, The Dash Core developers
// SPDX-License-Identifier: MIT
// See the accompanying file LICENSE or https://opensource.org/license/mit
//

use binrw::binread;
use binrw::io::Cursor;
use binrw::BinRead;

/// Maximum number of segments allowed in a wire-encoded path.
const MAX_PATH_SEGMENTS: u32 = 256;

/// A length-prefixed byte vector in the wire format (read-only).
///
/// Wire layout (little-endian): `[u32 len][bytes...]`
#[binread]
#[br(little)]
#[derive(Debug, Clone)]
struct WireBytes {
  #[br(temp)]
  len: u32,
  #[br(count = len)]
  data: Vec<u8>,
}

/// A wire-encoded path: a counted sequence of byte-vector segments (read-only).
///
/// Wire layout (little-endian): `[u32 count][WireBytes₁][WireBytes₂]…`
#[binread]
#[br(little)]
#[derive(Debug, Clone)]
struct WirePath {
  #[br(temp)]
  count: u32,
  #[br(count = count, assert(count <= MAX_PATH_SEGMENTS, "too many path segments: {}", count))]
  segments: Vec<WireBytes>,
}

/// Decode a flat-encoded path from C++.
///
/// Wire format (little-endian u32):
/// ```text
/// [segment_count][len₁][bytes₁][len₂][bytes₂]…
/// ```
pub(crate) fn decode_path(encoded: &[u8]) -> Result<Vec<Vec<u8>>, String> {
  let mut cursor = Cursor::new(encoded);
  let wire_path = WirePath::read_le(&mut cursor).map_err(crate::ffi_error_generic)?;
  Ok(
    wire_path
      .segments
      .into_iter()
      .map(|seg| seg.data)
      .collect(),
  )
}

/// Encode a path to the wire format.
///
/// Wire format (little-endian u32):
/// ```text
/// [segment_count][len₁][bytes₁][len₂][bytes₂]…
/// ```
#[cfg(test)]
fn encode_path(path: &[Vec<u8>]) -> Vec<u8> {
  let mut buf = Vec::new();
  buf.extend_from_slice(&(path.len() as u32).to_le_bytes());
  for seg in path {
    buf.extend_from_slice(&(seg.len() as u32).to_le_bytes());
    buf.extend_from_slice(seg);
  }
  buf
}

#[cfg(test)]
mod tests {
  use super::*;

  #[test]
  fn decode_empty_path() {
    let encoded = 0u32.to_le_bytes();
    let result = decode_path(&encoded).expect("should decode");
    assert!(result.is_empty());
  }

  #[test]
  fn decode_single_segment() {
    let mut buf = Vec::new();
    buf.extend_from_slice(&1u32.to_le_bytes());
    buf.extend_from_slice(&3u32.to_le_bytes());
    buf.extend_from_slice(b"abc");
    let result = decode_path(&buf).expect("should decode");
    assert_eq!(result, vec![b"abc".to_vec()]);
  }

  #[test]
  fn decode_multiple_segments() {
    let mut buf = Vec::new();
    buf.extend_from_slice(&3u32.to_le_bytes());
    buf.extend_from_slice(&2u32.to_le_bytes());
    buf.extend_from_slice(b"ab");
    buf.extend_from_slice(&0u32.to_le_bytes());
    buf.extend_from_slice(&4u32.to_le_bytes());
    buf.extend_from_slice(b"test");
    let result = decode_path(&buf).expect("should decode");
    assert_eq!(
      result,
      vec![b"ab".to_vec(), b"".to_vec(), b"test".to_vec()]
    );
  }

  #[test]
  fn decode_rejects_short_input() {
    assert!(decode_path(&[]).is_err());
    assert!(decode_path(&[1, 2]).is_err());
  }

  #[test]
  fn decode_rejects_truncated_segment() {
    let mut buf = Vec::new();
    buf.extend_from_slice(&1u32.to_le_bytes());
    // length header present but no payload
    buf.extend_from_slice(&10u32.to_le_bytes());
    assert!(decode_path(&buf).is_err());
  }

  #[test]
  fn encode_decode_roundtrip() {
    let path = vec![b"hello".to_vec(), b"world".to_vec()];
    let encoded = encode_path(&path);
    let decoded = decode_path(&encoded).expect("should decode");
    assert_eq!(decoded, path);
  }

  #[test]
  fn encode_empty_path() {
    let path: Vec<Vec<u8>> = vec![];
    let encoded = encode_path(&path);
    let decoded = decode_path(&encoded).expect("should decode");
    assert!(decoded.is_empty());
  }
}

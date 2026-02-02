//
// Copyright (c) 2026-present, The Dash Core developers
// SPDX-License-Identifier: MIT
// See the accompanying file LICENSE or https://opensource.org/license/mit
//

/// Maximum number of segments allowed in a wire-encoded path.
const MAX_PATH_SEGMENTS: u32 = 256;

/// Decode a flat-encoded path from C++.
///
/// Wire format (little-endian u32):
/// ```text
/// [segment_count][len₁][bytes₁][len₂][bytes₂]…
/// ```
pub(crate) fn decode_path(encoded: &[u8]) -> Result<Vec<Vec<u8>>, String> {
    if encoded.len() < 4 {
        return Err("path too short: missing segment count".into());
    }

    let count = u32::from_le_bytes(
        encoded[..4]
            .try_into()
            .map_err(|_| "failed to read segment count".to_string())?,
    );

    if count > MAX_PATH_SEGMENTS {
        return Err(format!(
            "segment count {count} exceeds maximum {MAX_PATH_SEGMENTS}"
        ));
    }

    let count = count as usize;
    let mut offset = 4usize;
    let mut segments = Vec::with_capacity(count);

    for i in 0..count {
        if offset + 4 > encoded.len() {
            return Err(format!(
                "path truncated at segment {i}: missing length"
            ));
        }

        let len = u32::from_le_bytes(
            encoded[offset..offset + 4]
                .try_into()
                .map_err(|_| format!("failed to read length of segment {i}"))?,
        ) as usize;
        offset += 4;

        if offset + len > encoded.len() {
            return Err(format!(
                "path truncated at segment {i}: need {len} bytes, have {}",
                encoded.len() - offset
            ));
        }

        segments.push(encoded[offset..offset + len].to_vec());
        offset += len;
    }

    Ok(segments)
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
}

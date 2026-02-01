//
// Copyright (c) 2026-present, The Dash Core developers
// SPDX-License-Identifier: MIT
// See the accompanying file LICENSE or https://opensource.org/license/mit
//

use grovedb::{PathQuery, Query, QueryItem, SizedQuery};
use grovedb_version::version::GroveVersion;

use crate::ffi::FfiQueryResult;
use crate::lifecycle::operation_cost_to_ffi;
use crate::types::decode_path;
use crate::BoxedGroveDb;
use crate::BoxedPathQuery;
use crate::BoxedTransaction;

// ---------------------------------------------------------------------------
// Query item descriptor wire format
// ---------------------------------------------------------------------------
//
// Wire format (all integers little-endian):
//
// [u32 item_count]
// For each item:
//   [u8 kind]
//     0 = Key            : [u32 len][bytes]
//     1 = Range           : [u32 start_len][start][u32 end_len][end]
//     2 = RangeInclusive  : [u32 start_len][start][u32 end_len][end]
//     3 = RangeFull       : (no data)
//     4 = RangeFrom       : [u32 start_len][start]
//     5 = RangeTo         : [u32 end_len][end]
//     6 = RangeToInclusive: [u32 end_len][end]
//     7 = RangeAfter      : [u32 after_len][after]
//     8 = RangeAfterTo    : [u32 after_len][after][u32 to_len][to]
//     9 = RangeAfterToInclusive : [u32 after_len][after][u32 to_len][to]

/// Read a u32 from the buffer at the given offset.
fn read_u32(buf: &[u8], offset: &mut usize) -> Result<u32, String> {
    if *offset + 4 > buf.len() {
        return Err(format!(
            "query descriptor truncated at offset {}: need 4 bytes for u32",
            *offset
        ));
    }
    let val = u32::from_le_bytes(
        buf[*offset..*offset + 4]
            .try_into()
            .map_err(|_| "failed to read u32".to_string())?,
    );
    *offset += 4;
    Ok(val)
}

/// Read a length-prefixed byte slice from the buffer.
fn read_bytes(buf: &[u8], offset: &mut usize) -> Result<Vec<u8>, String> {
    let len = read_u32(buf, offset)? as usize;
    if *offset + len > buf.len() {
        return Err(format!(
            "query descriptor truncated at offset {}: need {} bytes",
            *offset, len
        ));
    }
    let bytes = buf[*offset..*offset + len].to_vec();
    *offset += len;
    Ok(bytes)
}

/// Decode query items from the wire format into a vector of `QueryItem`.
fn decode_query_items(encoded: &[u8]) -> Result<Vec<QueryItem>, String> {
    let mut offset = 0usize;
    let count = read_u32(encoded, &mut offset)? as usize;
    let mut items = Vec::with_capacity(count);

    for i in 0..count {
        if offset >= encoded.len() {
            return Err(format!("query descriptor truncated at item {i}: missing kind byte"));
        }
        let kind = encoded[offset];
        offset += 1;

        let item = match kind {
            0 => {
                // Key
                let key = read_bytes(encoded, &mut offset)?;
                QueryItem::Key(key)
            }
            1 => {
                // Range (exclusive end)
                let start = read_bytes(encoded, &mut offset)?;
                let end = read_bytes(encoded, &mut offset)?;
                QueryItem::Range(start..end)
            }
            2 => {
                // RangeInclusive
                let start = read_bytes(encoded, &mut offset)?;
                let end = read_bytes(encoded, &mut offset)?;
                QueryItem::RangeInclusive(start..=end)
            }
            3 => {
                // RangeFull
                QueryItem::RangeFull(..)
            }
            4 => {
                // RangeFrom
                let start = read_bytes(encoded, &mut offset)?;
                QueryItem::RangeFrom(start..)
            }
            5 => {
                // RangeTo
                let end = read_bytes(encoded, &mut offset)?;
                QueryItem::RangeTo(..end)
            }
            6 => {
                // RangeToInclusive
                let end = read_bytes(encoded, &mut offset)?;
                QueryItem::RangeToInclusive(..=end)
            }
            7 => {
                // RangeAfter
                let after = read_bytes(encoded, &mut offset)?;
                QueryItem::RangeAfter(after..)
            }
            8 => {
                // RangeAfterTo
                let after = read_bytes(encoded, &mut offset)?;
                let to = read_bytes(encoded, &mut offset)?;
                QueryItem::RangeAfterTo(after..to)
            }
            9 => {
                // RangeAfterToInclusive
                let after = read_bytes(encoded, &mut offset)?;
                let to = read_bytes(encoded, &mut offset)?;
                QueryItem::RangeAfterToInclusive(after..=to)
            }
            _ => return Err(format!("unknown query item kind {kind} at item {i}")),
        };
        items.push(item);
    }

    Ok(items)
}

/// Encode query result values into the flat wire format.
///
/// Wire format: `[u32 count][u32 len₁][bytes₁][u32 len₂][bytes₂]…`
fn encode_values(values: &[Vec<u8>]) -> Vec<u8> {
    let total: usize = 4 + values.iter().map(|v| 4 + v.len()).sum::<usize>();
    let mut buf = Vec::with_capacity(total);

    buf.extend_from_slice(&(values.len() as u32).to_le_bytes());
    for value in values {
        buf.extend_from_slice(&(value.len() as u32).to_le_bytes());
        buf.extend_from_slice(value);
    }

    buf
}

// ---------------------------------------------------------------------------
// PathQuery factory
// ---------------------------------------------------------------------------

/// Create a `PathQuery` from wire-encoded path, query items, and limit/offset.
///
/// - `path`: wire-encoded path segments (same format as other bridge functions)
/// - `query_items`: wire-encoded query item descriptors
/// - `limit`: 0 means no limit
/// - `offset`: 0 means no offset
pub fn grovedb_path_query_new(
    path: &[u8],
    query_items: &[u8],
    limit: u32,
    offset: u32,
) -> Result<Box<BoxedPathQuery>, String> {
    let segments = decode_path(path)?;
    let items = decode_query_items(query_items)?;

    let mut query = Query::new();
    for item in items {
        query.items.push(item);
    }

    let limit = if limit == 0 { None } else { Some(limit as u16) };
    let offset = if offset == 0 { None } else { Some(offset as u16) };

    let sized_query = SizedQuery::new(query, limit, offset);
    let path_query = PathQuery::new(segments, sized_query);

    Ok(Box::new(BoxedPathQuery { query: path_query }))
}

/// Create a `PathQuery` from wire-encoded path, query items, and limit/offset.
///
/// This variant also accepts a subquery path and subquery items for the
/// default subquery branch, enabling hierarchical (two-level) queries.
///
/// - `subquery_path`: wire-encoded path segments for the subquery navigation
///   (empty [u32 0] means no subquery path)
/// - `subquery_items`: wire-encoded query items for the subquery
///   (empty [u32 0] means no subquery)
pub fn grovedb_path_query_new_with_subquery(
    path: &[u8],
    query_items: &[u8],
    limit: u32,
    offset: u32,
    subquery_path: &[u8],
    subquery_items: &[u8],
) -> Result<Box<BoxedPathQuery>, String> {
    let segments = decode_path(path)?;
    let items = decode_query_items(query_items)?;

    let mut query = Query::new();
    for item in items {
        query.items.push(item);
    }

    // Parse subquery path (if non-empty).
    let sq_path_segments = decode_path(subquery_path)?;
    let sq_path = if sq_path_segments.is_empty() {
        None
    } else {
        Some(sq_path_segments)
    };

    // Parse subquery items (if non-empty).
    let sq_items = decode_query_items(subquery_items)?;
    let sq_query = if sq_items.is_empty() {
        None
    } else {
        let mut sq = Query::new();
        for item in sq_items {
            sq.items.push(item);
        }
        Some(Box::new(sq))
    };

    if let Some(path) = sq_path {
        query.set_subquery_path(path);
    }
    if let Some(sq) = sq_query {
        query.set_subquery(*sq);
    }

    let limit = if limit == 0 { None } else { Some(limit as u16) };
    let offset = if offset == 0 { None } else { Some(offset as u16) };

    let sized_query = SizedQuery::new(query, limit, offset);
    let path_query = PathQuery::new(segments, sized_query);

    Ok(Box::new(BoxedPathQuery { query: path_query }))
}

// ---------------------------------------------------------------------------
// query_item_value — returns raw item bytes
// ---------------------------------------------------------------------------

/// Execute a `query_item_value` query and return the results.
pub fn grovedb_query_item_value(
    db: &BoxedGroveDb,
    query: &BoxedPathQuery,
) -> Result<FfiQueryResult, String> {
    let version = GroveVersion::latest();
    let ctx = db.db.query_item_value(
        &query.query,
        true,  // allow_cache
        true,  // decrease_limit_on_range_with_no_sub_elements
        true,  // error_if_intermediate_path_tree_not_present
        None,  // transaction
        version,
    );
    let cost = operation_cost_to_ffi(&ctx.cost);
    let (values, skipped) = ctx.value.map_err(|e| e.to_string())?;
    Ok(FfiQueryResult {
        values: encode_values(&values),
        skipped,
        cost,
    })
}

/// Execute a `query_item_value` query within a transaction.
pub fn grovedb_query_item_value_with_tx(
    db: &BoxedGroveDb,
    query: &BoxedPathQuery,
    tx: &BoxedTransaction,
) -> Result<FfiQueryResult, String> {
    let version = GroveVersion::latest();
    let ctx = db.db.query_item_value(
        &query.query,
        true,  // allow_cache
        true,  // decrease_limit_on_range_with_no_sub_elements
        true,  // error_if_intermediate_path_tree_not_present
        Some(&tx.tx),
        version,
    );
    let cost = operation_cost_to_ffi(&ctx.cost);
    let (values, skipped) = ctx.value.map_err(|e| e.to_string())?;
    Ok(FfiQueryResult {
        values: encode_values(&values),
        skipped,
        cost,
    })
}


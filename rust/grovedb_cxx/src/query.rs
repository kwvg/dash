//
// Copyright (c) 2026-present, The Dash Core developers
// SPDX-License-Identifier: MIT
// See the accompanying file LICENSE or https://opensource.org/license/mit
//

use grovedb::operations::QueryItemOrSumReturnType;
use grovedb::query_result_type::{
    PathKeyOptionalElementTrio, QueryResultElement, QueryResultElements, QueryResultType,
};
use grovedb::{PathQuery, Query, QueryItem, SizedQuery};
use grovedb_version::version::GroveVersion;

use crate::element::serialize_element;
use crate::ffi::{
    FfiQueryItemOrSumResult, FfiQueryKeysOptionalResult, FfiQueryRawResult, FfiQueryResult,
    FfiQuerySumsResult,
};
use crate::lifecycle::operation_cost_to_ffi;
use crate::types::decode_path;
use crate::BoxedGroveDb;
use crate::BoxedPathQuery;
use crate::BoxedPathQueryVec;
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

/// Convert a `usize` to `u32` for wire encoding, returning an error on overflow.
fn to_wire_u32(len: usize) -> Result<u32, String> {
    u32::try_from(len).map_err(|_| format!("wire encoding: length {len} exceeds u32::MAX"))
}

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
fn encode_values(values: &[Vec<u8>]) -> Result<Vec<u8>, String> {
    let total: usize = 4 + values.iter().map(|v| 4 + v.len()).sum::<usize>();
    let mut buf = Vec::with_capacity(total);

    buf.extend_from_slice(&to_wire_u32(values.len())?.to_le_bytes());
    for value in values {
        buf.extend_from_slice(&to_wire_u32(value.len())?.to_le_bytes());
        buf.extend_from_slice(value);
    }

    Ok(buf)
}

// ---------------------------------------------------------------------------
// BoxedPathQueryVec helpers
// ---------------------------------------------------------------------------

/// Create a new empty `BoxedPathQueryVec`.
pub fn grovedb_path_query_vec_new() -> Box<BoxedPathQueryVec> {
    Box::new(BoxedPathQueryVec {
        queries: Vec::new(),
    })
}

/// Push a clone of a `BoxedPathQuery` into a `BoxedPathQueryVec`.
pub fn grovedb_path_query_vec_push(vec: &mut BoxedPathQueryVec, query: &BoxedPathQuery) {
    vec.queries.push(query.query.clone());
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

    let limit = if limit == 0 {
        None
    } else if limit > u16::MAX as u32 {
        return Err(format!("limit {limit} exceeds u16::MAX ({})", u16::MAX));
    } else {
        Some(limit as u16)
    };
    let offset = if offset == 0 {
        None
    } else if offset > u16::MAX as u32 {
        return Err(format!("offset {offset} exceeds u16::MAX ({})", u16::MAX));
    } else {
        Some(offset as u16)
    };

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

    let limit = if limit == 0 {
        None
    } else if limit > u16::MAX as u32 {
        return Err(format!("limit {limit} exceeds u16::MAX ({})", u16::MAX));
    } else {
        Some(limit as u16)
    };
    let offset = if offset == 0 {
        None
    } else if offset > u16::MAX as u32 {
        return Err(format!("offset {offset} exceeds u16::MAX ({})", u16::MAX));
    } else {
        Some(offset as u16)
    };

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
        values: encode_values(&values)?,
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
        values: encode_values(&values)?,
        skipped,
        cost,
    })
}

// ---------------------------------------------------------------------------
// query_item_value_or_sum — returns tagged union of items/sums/counts
// ---------------------------------------------------------------------------
//
// Wire format for each entry:
//   [u8 tag][data]
//   tag=0 ItemData:          [u32 len][bytes]
//   tag=1 SumValue:          [i64 le]
//   tag=2 BigSumValue:       [i128 le] (16 bytes)
//   tag=3 CountValue:        [u64 le]
//   tag=4 CountSumValue:     [u64 count le][i64 sum le]
//   tag=5 ItemDataWithSum:   [u32 len][bytes][i64 sum le]
//
// Full result: [u32 count][entry₁][entry₂]…

/// Encode a vector of `QueryItemOrSumReturnType` into the wire format.
fn encode_item_or_sum_results(items: &[QueryItemOrSumReturnType]) -> Result<Vec<u8>, String> {
    let mut buf = Vec::new();
    buf.extend_from_slice(&to_wire_u32(items.len())?.to_le_bytes());

    for item in items {
        match item {
            QueryItemOrSumReturnType::ItemData(data) => {
                buf.push(0u8);
                buf.extend_from_slice(&to_wire_u32(data.len())?.to_le_bytes());
                buf.extend_from_slice(data);
            }
            QueryItemOrSumReturnType::SumValue(v) => {
                buf.push(1u8);
                buf.extend_from_slice(&v.to_le_bytes());
            }
            QueryItemOrSumReturnType::BigSumValue(v) => {
                buf.push(2u8);
                buf.extend_from_slice(&v.to_le_bytes());
            }
            QueryItemOrSumReturnType::CountValue(v) => {
                buf.push(3u8);
                buf.extend_from_slice(&v.to_le_bytes());
            }
            QueryItemOrSumReturnType::CountSumValue(count, sum) => {
                buf.push(4u8);
                buf.extend_from_slice(&count.to_le_bytes());
                buf.extend_from_slice(&sum.to_le_bytes());
            }
            QueryItemOrSumReturnType::ItemDataWithSumValue(data, sum) => {
                buf.push(5u8);
                buf.extend_from_slice(&to_wire_u32(data.len())?.to_le_bytes());
                buf.extend_from_slice(data);
                buf.extend_from_slice(&sum.to_le_bytes());
            }
        }
    }

    Ok(buf)
}

/// Execute a `query_item_value_or_sum` query.
pub fn grovedb_query_item_value_or_sum(
    db: &BoxedGroveDb,
    query: &BoxedPathQuery,
) -> Result<FfiQueryItemOrSumResult, String> {
    let version = GroveVersion::latest();
    let ctx = db.db.query_item_value_or_sum(
        &query.query,
        true,  // allow_cache
        true,  // decrease_limit_on_range_with_no_sub_elements
        true,  // error_if_intermediate_path_tree_not_present
        None,  // transaction
        version,
    );
    let cost = operation_cost_to_ffi(&ctx.cost);
    let (items, skipped) = ctx.value.map_err(|e| e.to_string())?;
    Ok(FfiQueryItemOrSumResult {
        values: encode_item_or_sum_results(&items)?,
        skipped,
        cost,
    })
}

/// Execute a `query_item_value_or_sum` query within a transaction.
pub fn grovedb_query_item_value_or_sum_with_tx(
    db: &BoxedGroveDb,
    query: &BoxedPathQuery,
    tx: &BoxedTransaction,
) -> Result<FfiQueryItemOrSumResult, String> {
    let version = GroveVersion::latest();
    let ctx = db.db.query_item_value_or_sum(
        &query.query,
        true,  // allow_cache
        true,  // decrease_limit_on_range_with_no_sub_elements
        true,  // error_if_intermediate_path_tree_not_present
        Some(&tx.tx),
        version,
    );
    let cost = operation_cost_to_ffi(&ctx.cost);
    let (items, skipped) = ctx.value.map_err(|e| e.to_string())?;
    Ok(FfiQueryItemOrSumResult {
        values: encode_item_or_sum_results(&items)?,
        skipped,
        cost,
    })
}

// ---------------------------------------------------------------------------
// query_sums — returns sum values only
// ---------------------------------------------------------------------------

/// Encode a vector of i64 sum values into the wire format.
///
/// Wire format: `[u32 count][i64₁ le][i64₂ le]…`
fn encode_sums(sums: &[i64]) -> Result<Vec<u8>, String> {
    let mut buf = Vec::with_capacity(4 + sums.len() * 8);
    buf.extend_from_slice(&to_wire_u32(sums.len())?.to_le_bytes());
    for sum in sums {
        buf.extend_from_slice(&sum.to_le_bytes());
    }
    Ok(buf)
}

/// Execute a `query_sums` query.
pub fn grovedb_query_sums(
    db: &BoxedGroveDb,
    query: &BoxedPathQuery,
) -> Result<FfiQuerySumsResult, String> {
    let version = GroveVersion::latest();
    let ctx = db.db.query_sums(
        &query.query,
        true,
        true,
        true,
        None,
        version,
    );
    let cost = operation_cost_to_ffi(&ctx.cost);
    let (sums, skipped) = ctx.value.map_err(|e| e.to_string())?;
    Ok(FfiQuerySumsResult {
        values: encode_sums(&sums)?,
        skipped,
        cost,
    })
}

/// Execute a `query_sums` query within a transaction.
pub fn grovedb_query_sums_with_tx(
    db: &BoxedGroveDb,
    query: &BoxedPathQuery,
    tx: &BoxedTransaction,
) -> Result<FfiQuerySumsResult, String> {
    let version = GroveVersion::latest();
    let ctx = db.db.query_sums(
        &query.query,
        true,
        true,
        true,
        Some(&tx.tx),
        version,
    );
    let cost = operation_cost_to_ffi(&ctx.cost);
    let (sums, skipped) = ctx.value.map_err(|e| e.to_string())?;
    Ok(FfiQuerySumsResult {
        values: encode_sums(&sums)?,
        skipped,
        cost,
    })
}

// ---------------------------------------------------------------------------
// query_raw — returns full Element structures
// ---------------------------------------------------------------------------
//
// Wire format per element depends on QueryResultType:
//   variant=0 Element:            [element bincode bytes]
//   variant=1 KeyElementPair:     [u32 key_len][key][element bytes]
//   variant=2 PathKeyElementTrio: [u32 seg_count][seg₁]…[u32 key_len][key][element bytes]
//
// Full result: [u32 count][entry₁][entry₂]…

/// Encode a `QueryResultElements` into the wire format.
fn encode_query_result_elements(elements: &QueryResultElements) -> Result<Vec<u8>, String> {
    let version = GroveVersion::latest();
    let mut buf = Vec::new();
    buf.extend_from_slice(&to_wire_u32(elements.elements.len())?.to_le_bytes());

    for elem in &elements.elements {
        match elem {
            QueryResultElement::ElementResultItem(element) => {
                buf.push(0u8);
                let elem_bytes = serialize_element(element, version)?;
                buf.extend_from_slice(&to_wire_u32(elem_bytes.len())?.to_le_bytes());
                buf.extend_from_slice(&elem_bytes);
            }
            QueryResultElement::KeyElementPairResultItem((key, element)) => {
                buf.push(1u8);
                buf.extend_from_slice(&to_wire_u32(key.len())?.to_le_bytes());
                buf.extend_from_slice(key);
                let elem_bytes = serialize_element(element, version)?;
                buf.extend_from_slice(&to_wire_u32(elem_bytes.len())?.to_le_bytes());
                buf.extend_from_slice(&elem_bytes);
            }
            QueryResultElement::PathKeyElementTrioResultItem((path, key, element)) => {
                buf.push(2u8);
                // Encode path
                buf.extend_from_slice(&to_wire_u32(path.len())?.to_le_bytes());
                for seg in path {
                    buf.extend_from_slice(&to_wire_u32(seg.len())?.to_le_bytes());
                    buf.extend_from_slice(seg);
                }
                // Encode key
                buf.extend_from_slice(&to_wire_u32(key.len())?.to_le_bytes());
                buf.extend_from_slice(key);
                // Encode element
                let elem_bytes = serialize_element(element, version)?;
                buf.extend_from_slice(&to_wire_u32(elem_bytes.len())?.to_le_bytes());
                buf.extend_from_slice(&elem_bytes);
            }
        }
    }

    Ok(buf)
}

/// Execute a `query_raw` query returning `QueryElementResultType` results.
pub fn grovedb_query_raw(
    db: &BoxedGroveDb,
    query: &BoxedPathQuery,
    result_type: u8,
) -> Result<FfiQueryRawResult, String> {
    let version = GroveVersion::latest();
    let rt = match result_type {
        0 => QueryResultType::QueryElementResultType,
        1 => QueryResultType::QueryKeyElementPairResultType,
        2 => QueryResultType::QueryPathKeyElementTrioResultType,
        _ => return Err(format!("unknown query result type: {result_type}")),
    };
    let ctx = db.db.query_raw(
        &query.query,
        true,
        true,
        true,
        rt,
        None,
        version,
    );
    let cost = operation_cost_to_ffi(&ctx.cost);
    let (elements, skipped) = ctx.value.map_err(|e| e.to_string())?;
    Ok(FfiQueryRawResult {
        values: encode_query_result_elements(&elements)?,
        skipped,
        cost,
    })
}

/// Execute a `query_raw` query within a transaction.
pub fn grovedb_query_raw_with_tx(
    db: &BoxedGroveDb,
    query: &BoxedPathQuery,
    result_type: u8,
    tx: &BoxedTransaction,
) -> Result<FfiQueryRawResult, String> {
    let version = GroveVersion::latest();
    let rt = match result_type {
        0 => QueryResultType::QueryElementResultType,
        1 => QueryResultType::QueryKeyElementPairResultType,
        2 => QueryResultType::QueryPathKeyElementTrioResultType,
        _ => return Err(format!("unknown query result type: {result_type}")),
    };
    let ctx = db.db.query_raw(
        &query.query,
        true,
        true,
        true,
        rt,
        Some(&tx.tx),
        version,
    );
    let cost = operation_cost_to_ffi(&ctx.cost);
    let (elements, skipped) = ctx.value.map_err(|e| e.to_string())?;
    Ok(FfiQueryRawResult {
        values: encode_query_result_elements(&elements)?,
        skipped,
        cost,
    })
}

/// Execute a `query_many_raw` query across multiple path queries.
///
/// The `encoded_queries` parameter is a wire-encoded list of path queries:
/// ```text
/// [u32 query_count]
/// For each query:
///   [path wire][query_items wire][u32 limit][u32 offset]
/// ```
pub fn grovedb_query_many_raw(
    db: &BoxedGroveDb,
    encoded_queries: &[u8],
    result_type: u8,
) -> Result<FfiQueryRawResult, String> {
    let version = GroveVersion::latest();
    let rt = match result_type {
        0 => QueryResultType::QueryElementResultType,
        1 => QueryResultType::QueryKeyElementPairResultType,
        2 => QueryResultType::QueryPathKeyElementTrioResultType,
        _ => return Err(format!("unknown query result type: {result_type}")),
    };

    // Decode the queries from the wire format.
    let queries = decode_path_queries(encoded_queries)?;
    let refs: Vec<&PathQuery> = queries.iter().collect();

    let ctx = db.db.query_many_raw(
        &refs,
        true,
        true,
        true,
        rt,
        None,
        version,
    );
    let cost = operation_cost_to_ffi(&ctx.cost);
    let elements = ctx.value.map_err(|e| e.to_string())?;
    Ok(FfiQueryRawResult {
        values: encode_query_result_elements(&elements)?,
        skipped: 0,
        cost,
    })
}

/// Decode a wire-encoded list of PathQuery objects.
///
/// Wire format:
/// ```text
/// [u32 query_count]
/// For each query:
///   [path wire: u32 seg_count + segments][query_items wire: u32 count + items][u32 limit][u32 offset]
/// ```
fn decode_path_queries(encoded: &[u8]) -> Result<Vec<PathQuery>, String> {
    let mut offset = 0usize;
    let count = read_u32(encoded, &mut offset)? as usize;
    let mut queries = Vec::with_capacity(count);

    for i in 0..count {
        // Decode path segments inline.
        let seg_count = read_u32(encoded, &mut offset)? as usize;
        let mut segments = Vec::with_capacity(seg_count);
        for _ in 0..seg_count {
            let seg = read_bytes(encoded, &mut offset)?;
            segments.push(seg);
        }

        // Decode query items inline.
        let item_count = read_u32(encoded, &mut offset)? as usize;
        let mut query = Query::new();
        for j in 0..item_count {
            if offset >= encoded.len() {
                return Err(format!(
                    "query {i}: item {j}: truncated, missing kind byte"
                ));
            }
            let kind = encoded[offset];
            offset += 1;

            let item = match kind {
                0 => QueryItem::Key(read_bytes(encoded, &mut offset)?),
                1 => {
                    let start = read_bytes(encoded, &mut offset)?;
                    let end = read_bytes(encoded, &mut offset)?;
                    QueryItem::Range(start..end)
                }
                2 => {
                    let start = read_bytes(encoded, &mut offset)?;
                    let end = read_bytes(encoded, &mut offset)?;
                    QueryItem::RangeInclusive(start..=end)
                }
                3 => QueryItem::RangeFull(..),
                4 => QueryItem::RangeFrom(read_bytes(encoded, &mut offset)?..),
                5 => QueryItem::RangeTo(..read_bytes(encoded, &mut offset)?),
                6 => QueryItem::RangeToInclusive(..=read_bytes(encoded, &mut offset)?),
                7 => QueryItem::RangeAfter(read_bytes(encoded, &mut offset)?..),
                8 => {
                    let after = read_bytes(encoded, &mut offset)?;
                    let to = read_bytes(encoded, &mut offset)?;
                    QueryItem::RangeAfterTo(after..to)
                }
                9 => {
                    let after = read_bytes(encoded, &mut offset)?;
                    let to = read_bytes(encoded, &mut offset)?;
                    QueryItem::RangeAfterToInclusive(after..=to)
                }
                _ => {
                    return Err(format!(
                        "query {i}: item {j}: unknown kind {kind}"
                    ))
                }
            };
            query.items.push(item);
        }

        let limit = read_u32(encoded, &mut offset)?;
        let offset_val = read_u32(encoded, &mut offset)?;

        let limit = if limit == 0 {
            None
        } else if limit > u16::MAX as u32 {
            return Err(format!("query {i}: limit {limit} exceeds u16::MAX ({})", u16::MAX));
        } else {
            Some(limit as u16)
        };
        let offset_opt = if offset_val == 0 {
            None
        } else if offset_val > u16::MAX as u32 {
            return Err(format!("query {i}: offset {offset_val} exceeds u16::MAX ({})", u16::MAX));
        } else {
            Some(offset_val as u16)
        };

        let sized_query = SizedQuery::new(query, limit, offset_opt);
        queries.push(PathQuery::new(segments, sized_query));
    }

    Ok(queries)
}

// ---------------------------------------------------------------------------
// query_keys_optional / query_raw_keys_optional
// ---------------------------------------------------------------------------
//
// Wire format:
// [u32 count]
// For each triple:
//   [u32 seg_count][seg₁]…[segₙ]  (path encoding)
//   [u32 key_len][key]
//   [u8 has_element]  (0=None, 1=Some)
//   If has_element: [u32 elem_len][elem bincode]

/// Encode a vector of `PathKeyOptionalElementTrio` into the wire format.
fn encode_path_key_element_triples(
    entries: Vec<PathKeyOptionalElementTrio>,
) -> Result<Vec<u8>, String> {
    let version = GroveVersion::latest();
    let mut buf = Vec::new();
    buf.extend_from_slice(&to_wire_u32(entries.len())?.to_le_bytes());

    for (path, key, opt_element) in entries {
        // path
        buf.extend_from_slice(&to_wire_u32(path.len())?.to_le_bytes());
        for seg in &path {
            buf.extend_from_slice(&to_wire_u32(seg.len())?.to_le_bytes());
            buf.extend_from_slice(seg);
        }
        // key
        buf.extend_from_slice(&to_wire_u32(key.len())?.to_le_bytes());
        buf.extend_from_slice(&key);
        // optional element
        match opt_element {
            Some(element) => {
                buf.push(1u8);
                let elem_bytes = serialize_element(&element, version)?;
                buf.extend_from_slice(&to_wire_u32(elem_bytes.len())?.to_le_bytes());
                buf.extend_from_slice(&elem_bytes);
            }
            None => {
                buf.push(0u8);
            }
        }
    }

    Ok(buf)
}

/// Execute a `query_keys_optional` query.
pub fn grovedb_query_keys_optional(
    db: &BoxedGroveDb,
    query: &BoxedPathQuery,
) -> Result<FfiQueryKeysOptionalResult, String> {
    let version = GroveVersion::latest();
    let ctx = db.db.query_keys_optional(
        &query.query,
        true,
        true,
        true,
        None,
        version,
    );
    let cost = operation_cost_to_ffi(&ctx.cost);
    let entries = ctx.value.map_err(|e| e.to_string())?;
    Ok(FfiQueryKeysOptionalResult {
        values: encode_path_key_element_triples(entries)?,
        cost,
    })
}

/// Execute a `query_keys_optional` query within a transaction.
pub fn grovedb_query_keys_optional_with_tx(
    db: &BoxedGroveDb,
    query: &BoxedPathQuery,
    tx: &BoxedTransaction,
) -> Result<FfiQueryKeysOptionalResult, String> {
    let version = GroveVersion::latest();
    let ctx = db.db.query_keys_optional(
        &query.query,
        true,
        true,
        true,
        Some(&tx.tx),
        version,
    );
    let cost = operation_cost_to_ffi(&ctx.cost);
    let entries = ctx.value.map_err(|e| e.to_string())?;
    Ok(FfiQueryKeysOptionalResult {
        values: encode_path_key_element_triples(entries)?,
        cost,
    })
}

/// Execute a `query_raw_keys_optional` query (no reference following).
pub fn grovedb_query_raw_keys_optional(
    db: &BoxedGroveDb,
    query: &BoxedPathQuery,
) -> Result<FfiQueryKeysOptionalResult, String> {
    let version = GroveVersion::latest();
    let ctx = db.db.query_raw_keys_optional(
        &query.query,
        true,
        true,
        true,
        None,
        version,
    );
    let cost = operation_cost_to_ffi(&ctx.cost);
    let entries = ctx.value.map_err(|e| e.to_string())?;
    Ok(FfiQueryKeysOptionalResult {
        values: encode_path_key_element_triples(entries)?,
        cost,
    })
}

/// Execute a `query_raw_keys_optional` query within a transaction.
pub fn grovedb_query_raw_keys_optional_with_tx(
    db: &BoxedGroveDb,
    query: &BoxedPathQuery,
    tx: &BoxedTransaction,
) -> Result<FfiQueryKeysOptionalResult, String> {
    let version = GroveVersion::latest();
    let ctx = db.db.query_raw_keys_optional(
        &query.query,
        true,
        true,
        true,
        Some(&tx.tx),
        version,
    );
    let cost = operation_cost_to_ffi(&ctx.cost);
    let entries = ctx.value.map_err(|e| e.to_string())?;
    Ok(FfiQueryKeysOptionalResult {
        values: encode_path_key_element_triples(entries)?,
        cost,
    })
}


// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <db_internal.h>
#include <types/transaction.h>

#include <grovedb/batch.h>
#include <grovedb/wire.h>

#include <rust/grovedb_cxx/lib.h>

#include <cstdint>
#include <utility>
#include <vector>

namespace grovedb {
// ---------------------------------------------------------------------------
// BatchOperation factories
// ---------------------------------------------------------------------------

BatchOperation
BatchOperation::InsertOnly(const Path& path, const Bytes& key, const Element& element)
{
  BatchOperation op;
  op.m_kind = Kind::InsertOnly;
  op.m_path = path;
  op.m_key = key;
  op.m_element = element;
  return op;
}

BatchOperation
BatchOperation::InsertOrReplace(const Path& path, const Bytes& key, const Element& element)
{
  BatchOperation op;
  op.m_kind = Kind::InsertOrReplace;
  op.m_path = path;
  op.m_key = key;
  op.m_element = element;
  return op;
}

BatchOperation BatchOperation::Replace(const Path& path, const Bytes& key, const Element& element)
{
  BatchOperation op;
  op.m_kind = Kind::Replace;
  op.m_path = path;
  op.m_key = key;
  op.m_element = element;
  return op;
}

BatchOperation BatchOperation::Delete(const Path& path, const Bytes& key)
{
  BatchOperation op;
  op.m_kind = Kind::Delete;
  op.m_path = path;
  op.m_key = key;
  return op;
}

BatchOperation BatchOperation::DeleteTree(const Path& path, const Bytes& key, TreeType tree_type)
{
  BatchOperation op;
  op.m_kind = Kind::DeleteTree;
  op.m_path = path;
  op.m_key = key;
  op.m_tree_type = tree_type;
  return op;
}

// ---------------------------------------------------------------------------
// BatchOperation::Encode
// ---------------------------------------------------------------------------

void BatchOperation::Encode(wire::Writer& w) const
{
  // Path.
  wire::Write(w, m_path);

  // Key.
  w.Bytes(m_key);

  // Op discriminant.
  w.U8(static_cast<uint8_t>(m_kind));

  // Op-specific payload.
  switch (m_kind) {
  case Kind::InsertOnly:
  case Kind::InsertOrReplace:
  case Kind::Replace:
    w.Bytes(m_element.data());
    break;
  case Kind::Delete:
    // No additional payload.
    break;
  case Kind::DeleteTree:
    w.U8(static_cast<uint8_t>(m_tree_type));
    break;
  }
}

// ---------------------------------------------------------------------------
// Db::ApplyBatch
// ---------------------------------------------------------------------------

namespace {
/**
 * Encode a vector of batch operations to the wire format expected by the
 * Rust FFI: `[u32 count][WireBatchOp₁][WireBatchOp₂]…`.
 */
std::vector<uint8_t> EncodeBatchOps(const std::vector<BatchOperation>& ops)
{
  return wire::Encode(ops);
}

/**
 * Convert C++ BatchApplyOptions to the FFI struct.
 */
grovedb_cxx::FfiBatchApplyOptions ConvertOptions(const BatchApplyOptions& opts)
{
  return grovedb_cxx::FfiBatchApplyOptions{
      .validate_insertion_does_not_override = opts.m_validate_insertion_does_not_override,
      .validate_insertion_does_not_override_tree = opts.m_validate_insertion_does_not_override_tree,
      .allow_deleting_non_empty_trees = opts.m_allow_deleting_non_empty_trees,
      .deleting_non_empty_trees_returns_error = opts.m_deleting_non_empty_trees_returns_error,
      .disable_operation_consistency_check = opts.m_disable_operation_consistency_check,
      .base_root_storage_is_free = opts.m_base_root_storage_is_free,
  };
}
} // anonymous namespace

Result<OperationCost, Error>
Db::ApplyBatch(const std::vector<BatchOperation>& ops, const BatchApplyOptions& options)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto encoded = EncodeBatchOps(ops);
    rust::Slice<const uint8_t> ops_slice{encoded.data(), encoded.size()};
    auto ffi_opts = ConvertOptions(options);

    auto result = grovedb_cxx::grovedb_apply_batch(*m_impl->m_db, ops_slice, ffi_opts);
    return convert_cost(result);
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<OperationCost, Error> Db::ApplyBatch(
    const std::vector<BatchOperation>& ops, const BatchApplyOptions& options, const Transaction& txn
)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto encoded = EncodeBatchOps(ops);
    rust::Slice<const uint8_t> ops_slice{encoded.data(), encoded.size()};
    auto ffi_opts = ConvertOptions(options);

    auto result = grovedb_cxx::grovedb_apply_batch_with_tx(
        *m_impl->m_db, ops_slice, ffi_opts, *txn.m_impl->m_tx
    );
    return convert_cost(result);
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}
} // namespace grovedb

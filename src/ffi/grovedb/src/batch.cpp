// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifdef HAVE_GROVEDB_CONFIG
#include <config/grovedb-config.h>
#endif

#include "db_internal.h"

#include <grovedb/wire.h>

#include <types/transaction.h>

namespace grovedb {

// ---------------------------------------------------------------------------
// BatchOperation factories
// ---------------------------------------------------------------------------

BatchOperation BatchOperation::InsertOnly(Path path, Bytes key, Element element)
{
    BatchOperation op;
    op.m_path = std::move(path);
    op.m_key = std::move(key);
    op.m_kind = Kind::kInsertOnly;
    op.m_element = std::move(element);
    return op;
}

BatchOperation BatchOperation::InsertOrReplace(Path path, Bytes key, Element element)
{
    BatchOperation op;
    op.m_path = std::move(path);
    op.m_key = std::move(key);
    op.m_kind = Kind::kInsertOrReplace;
    op.m_element = std::move(element);
    return op;
}

BatchOperation BatchOperation::Replace(Path path, Bytes key, Element element)
{
    BatchOperation op;
    op.m_path = std::move(path);
    op.m_key = std::move(key);
    op.m_kind = Kind::kReplace;
    op.m_element = std::move(element);
    return op;
}

BatchOperation BatchOperation::Delete(Path path, Bytes key)
{
    BatchOperation op;
    op.m_path = std::move(path);
    op.m_key = std::move(key);
    op.m_kind = Kind::kDelete;
    return op;
}

BatchOperation BatchOperation::DeleteTree(Path path, Bytes key, TreeType tree_type)
{
    BatchOperation op;
    op.m_path = std::move(path);
    op.m_key = std::move(key);
    op.m_kind = Kind::kDeleteTree;
    op.m_tree_type = tree_type;
    return op;
}

// ---------------------------------------------------------------------------
// Batch encoding
// ---------------------------------------------------------------------------

Bytes Db::EncodeBatchOps(const std::vector<BatchOperation>& ops)
{
    wire::Writer w;
    w.U32(static_cast<uint32_t>(ops.size()));
    for (const auto& op : ops) {
        wire::WireWrite(w, op.m_path);
        w.Bytes(op.m_key);
        w.U8(static_cast<uint8_t>(op.m_kind));
        switch (op.m_kind) {
        case BatchOperation::Kind::kInsertOnly:
        case BatchOperation::Kind::kInsertOrReplace:
        case BatchOperation::Kind::kReplace:
            w.Bytes(op.m_element.m_data);
            break;
        case BatchOperation::Kind::kDeleteTree:
            w.U8(static_cast<uint8_t>(op.m_tree_type));
            break;
        case BatchOperation::Kind::kDelete:
            break;
        }
    }
    return w.Take();
}

// ---------------------------------------------------------------------------
// ApplyBatch
// ---------------------------------------------------------------------------

Status Db::ApplyBatch(const std::vector<BatchOperation>& ops,
                      OperationCost& cost)
{
    return ApplyBatch(ops, BatchApplyOptions{}, cost);
}

Status Db::ApplyBatch(const std::vector<BatchOperation>& ops,
                      const BatchApplyOptions& options,
                      OperationCost& cost)
{
    try {
        auto buf = EncodeBatchOps(ops);
        rust::Slice<const uint8_t> ops_slice{buf.data(), buf.size()};
        grovedb_cxx::FfiBatchApplyOptions ffi_opts{
            options.m_validate_insertion_does_not_override,
            options.m_validate_insertion_does_not_override_tree,
            options.m_allow_deleting_non_empty_trees,
            options.m_deleting_non_empty_trees_returns_error,
            options.m_disable_operation_consistency_check,
            options.m_base_root_storage_is_free,
        };
        auto result = grovedb_cxx::grovedb_apply_batch(
            *m_impl->m_db, ops_slice, ffi_opts);
        cost = convert_cost(result);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::ApplyBatch(const std::vector<BatchOperation>& ops,
                      const Transaction& txn,
                      OperationCost& cost)
{
    return ApplyBatch(ops, BatchApplyOptions{}, txn, cost);
}

Status Db::ApplyBatch(const std::vector<BatchOperation>& ops,
                      const BatchApplyOptions& options,
                      const Transaction& txn,
                      OperationCost& cost)
{
    try {
        auto buf = EncodeBatchOps(ops);
        rust::Slice<const uint8_t> ops_slice{buf.data(), buf.size()};
        grovedb_cxx::FfiBatchApplyOptions ffi_opts{
            options.m_validate_insertion_does_not_override,
            options.m_validate_insertion_does_not_override_tree,
            options.m_allow_deleting_non_empty_trees,
            options.m_deleting_non_empty_trees_returns_error,
            options.m_disable_operation_consistency_check,
            options.m_base_root_storage_is_free,
        };
        auto result = grovedb_cxx::grovedb_apply_batch_with_tx(
            *m_impl->m_db, ops_slice, ffi_opts, *txn.m_impl->m_tx);
        cost = convert_cost(result);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

} // namespace grovedb

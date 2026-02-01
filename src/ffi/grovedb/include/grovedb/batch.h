// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GROVEDB_BATCH_H
#define GROVEDB_BATCH_H

#include <grovedb/element.h>
#include <grovedb/types.h>

#include <cstdint>

namespace grovedb {

/** Tree type discriminant for DeleteTree operations. */
enum class TreeType : uint8_t {
    kNormalTree = 0,
    kSumTree = 1,
    kBigSumTree = 2,
    kCountTree = 3,
    kCountSumTree = 4,
    kProvableCountTree = 5,
    kProvableCountSumTree = 6,
};

/** Options controlling batch application behavior. */
struct BatchApplyOptions {
    bool m_validate_insertion_does_not_override{false};
    bool m_validate_insertion_does_not_override_tree{false};
    bool m_allow_deleting_non_empty_trees{false};
    bool m_deleting_non_empty_trees_returns_error{true};
    bool m_disable_operation_consistency_check{false};
    bool m_base_root_storage_is_free{true};
};

/**
 * A single operation within an atomic batch.
 *
 * Use the static factory methods to construct operations, then pass a
 * vector of them to Db::ApplyBatch().
 */
class BatchOperation
{
public:
    /** Insert element (must not already exist). */
    static BatchOperation InsertOnly(Path path, Bytes key, Element element);

    /** Insert or replace element (idempotent). */
    static BatchOperation InsertOrReplace(Path path, Bytes key, Element element);

    /** Replace element (must already exist). */
    static BatchOperation Replace(Path path, Bytes key, Element element);

    /** Delete an element. */
    static BatchOperation Delete(Path path, Bytes key);

    /** Delete a tree element. */
    static BatchOperation DeleteTree(Path path, Bytes key,
                                     TreeType tree_type = TreeType::kNormalTree);

private:
    friend class Db;

    enum class Kind : uint8_t {
        kInsertOnly = 0,
        kInsertOrReplace = 1,
        kReplace = 2,
        kDelete = 3,
        kDeleteTree = 4,
    };

    Path m_path;
    Bytes m_key;
    Kind m_kind;
    Element m_element;
    TreeType m_tree_type{TreeType::kNormalTree};
};

} // namespace grovedb

#endif // GROVEDB_BATCH_H

// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#ifndef LIBGROVEDB_BATCH_H
#define LIBGROVEDB_BATCH_H

#include <grovedb/element.h>
#include <grovedb/types.h>
#include <grovedb/wire.h>

#include <cstdint>
#include <ostream>
#include <string_view>
#include <utility>
#include <vector>

namespace grovedb {
/** @addtogroup batch
 *  @{ */
// ---------------------------------------------------------------------------
// TreeType — type of tree for DeleteTree operations
// ---------------------------------------------------------------------------

/** Tree type discriminant (matches Rust `TreeType`). */
enum class TreeType : uint8_t {
  NormalTree = 0,
  SumTree = 1,
  BigSumTree = 2,
  CountTree = 3,
  CountSumTree = 4,
  ProvableCountTree = 5,
  ProvableCountSumTree = 6,
};

/** @return String representation of the tree type. */
constexpr std::string_view ToString(TreeType tt)
{
  switch (tt) {
  case TreeType::NormalTree:
    return "NormalTree";
  case TreeType::SumTree:
    return "SumTree";
  case TreeType::BigSumTree:
    return "BigSumTree";
  case TreeType::CountTree:
    return "CountTree";
  case TreeType::CountSumTree:
    return "CountSumTree";
  case TreeType::ProvableCountTree:
    return "ProvableCountTree";
  case TreeType::ProvableCountSumTree:
    return "ProvableCountSumTree";
  } // no default case, so the compiler can warn about missing cases
  return "Unknown";
}

// ---------------------------------------------------------------------------
// BatchOperation — a single operation within a batch
// ---------------------------------------------------------------------------

/**
 * A single operation within an atomic batch.
 *
 * Use the static factory methods to construct instances.
 * Call `Encode(Writer&)` to serialize for FFI transport.
 */
class BatchOperation
{
public:
  /** Operation kind discriminant (wire format tag). */
  enum class Kind : uint8_t {
    InsertOnly = 0,
    InsertOrReplace = 1,
    Replace = 2,
    Delete = 3,
    DeleteTree = 4,
  };

  /** @name Factories */
  ///@{

  /** Insert a new element; fail if the key already exists. */
  static BatchOperation InsertOnly(const Path& path, const Bytes& key, const Element& element);
  /** Insert an element, overwriting any existing value. */
  static BatchOperation InsertOrReplace(const Path& path, const Bytes& key, const Element& element);
  /** Replace an existing element; fail if the key does not exist. */
  static BatchOperation Replace(const Path& path, const Bytes& key, const Element& element);
  /** Delete the element at the given path and key. */
  static BatchOperation Delete(const Path& path, const Bytes& key);
  /** Delete an entire subtree at the given path and key. */
  static BatchOperation
  DeleteTree(const Path& path, const Bytes& key, TreeType tree_type = TreeType::NormalTree);

  ///@}

  /** @return The operation kind. */
  Kind kind() const
  {
    return m_kind;
  }
  /** @return The operation's target path. */
  const Path& path() const
  {
    return m_path;
  }
  /** @return The operation's target key. */
  const Bytes& key() const
  {
    return m_key;
  }
  /** @return The element (for insert/replace ops). */
  const Element& element() const
  {
    return m_element;
  }
  /** @return The tree type (for DeleteTree ops). */
  TreeType tree_type() const
  {
    return m_tree_type;
  }

  /**
   * Serialize this operation to a wire::Writer.
   *
   * Wire layout:
   * ```
   * [WirePath path]
   * [u32 key_len][key_bytes]
   * [u8 op_discriminant]
   * -- for insert ops (0,1,2): [u32 elem_len][elem_bytes]
   * -- for delete_tree (4): [u8 tree_type]
   * -- for delete (3): nothing
   * ```
   *
   * @param[in] w  Writer to append to.
   */
  void Encode(wire::Writer& w) const;

  /** Default-construct an empty BatchOperation. */
  BatchOperation() = default;

private:
  Kind m_kind{Kind::InsertOnly};
  Path m_path;
  Bytes m_key;
  Element m_element;
  TreeType m_tree_type{TreeType::NormalTree};
};

/** @return String representation of the operation kind. */
constexpr std::string_view ToString(BatchOperation::Kind kind)
{
  switch (kind) {
  case BatchOperation::Kind::InsertOnly:
    return "InsertOnly";
  case BatchOperation::Kind::InsertOrReplace:
    return "InsertOrReplace";
  case BatchOperation::Kind::Replace:
    return "Replace";
  case BatchOperation::Kind::Delete:
    return "Delete";
  case BatchOperation::Kind::DeleteTree:
    return "DeleteTree";
  } // no default case, so the compiler can warn about missing cases
  return "Unknown";
}

// ---------------------------------------------------------------------------
// BatchApplyOptions — controls batch validation behavior
// ---------------------------------------------------------------------------

/**
 * Options controlling batch application behavior.
 *
 * All fields default to the same values as Rust `BatchApplyOptions::default()`.
 */
struct BatchApplyOptions {
  bool m_validate_insertion_does_not_override{false};
  bool m_validate_insertion_does_not_override_tree{false};
  bool m_allow_deleting_non_empty_trees{false};
  bool m_deleting_non_empty_trees_returns_error{true};
  bool m_disable_operation_consistency_check{false};
  bool m_base_root_storage_is_free{true};
};

// ---------------------------------------------------------------------------
// Wire ADL: vector<BatchOperation>
// ---------------------------------------------------------------------------

namespace wire {
/**
 * Encode a single BatchOperation to a wire::Writer.
 *
 * @param[in] w   Writer to append to.
 * @param[in] op  BatchOperation to encode.
 */
inline void Write(Writer& w, const BatchOperation& op)
{
  op.Encode(w);
}
} // namespace wire

/** @brief Stream insertion for TreeType. */
inline std::ostream& operator<<(std::ostream& os, TreeType tt)
{
  return os << ToString(tt);
}

/** @brief Stream insertion for BatchOperation::Kind. */
inline std::ostream& operator<<(std::ostream& os, BatchOperation::Kind kind)
{
  return os << ToString(kind);
}
/** @} */
} // namespace grovedb

#endif // LIBGROVEDB_BATCH_H

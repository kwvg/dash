// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <db_internal.h>
#include <types/transaction.h>

#include <grovedb/wire.h>

#include <rust/grovedb_cxx/lib.h>

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace grovedb {
// ---------------------------------------------------------------------------
// PutAux
// ---------------------------------------------------------------------------

Result<OperationCost, Error> Db::PutAux(const Bytes& key, const Bytes& value)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};
    rust::Slice<const uint8_t> val_slice{value.data(), value.size()};

    auto result = grovedb_cxx::grovedb_put_aux(*m_impl->m_db, key_slice, val_slice);
    return convert_cost(result);
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<OperationCost, Error>
Db::PutAux(const Bytes& key, const Bytes& value, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};
    rust::Slice<const uint8_t> val_slice{value.data(), value.size()};

    auto result = grovedb_cxx::grovedb_put_aux_with_tx(
        *m_impl->m_db, key_slice, val_slice, *txn.m_impl->m_tx
    );
    return convert_cost(result);
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

// ---------------------------------------------------------------------------
// GetAux
// ---------------------------------------------------------------------------

Result<Costed<std::optional<Bytes>>, Error> Db::GetAux(const Bytes& key)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

    auto result = grovedb_cxx::grovedb_get_aux(*m_impl->m_db, key_slice);
    auto cost = convert_cost(result.cost);
    if (result.has_value) {
      Bytes val{result.value.begin(), result.value.end()};
      return Costed<std::optional<Bytes>>{std::move(val), cost};
    }
    return Costed<std::optional<Bytes>>{std::nullopt, cost};
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<Costed<std::optional<Bytes>>, Error> Db::GetAux(const Bytes& key, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

    auto result = grovedb_cxx::grovedb_get_aux_with_tx(*m_impl->m_db, key_slice, *txn.m_impl->m_tx);
    auto cost = convert_cost(result.cost);
    if (result.has_value) {
      Bytes val{result.value.begin(), result.value.end()};
      return Costed<std::optional<Bytes>>{std::move(val), cost};
    }
    return Costed<std::optional<Bytes>>{std::nullopt, cost};
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

// ---------------------------------------------------------------------------
// DeleteAux
// ---------------------------------------------------------------------------

Result<OperationCost, Error> Db::DeleteAux(const Bytes& key)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

    auto result = grovedb_cxx::grovedb_delete_aux(*m_impl->m_db, key_slice);
    return convert_cost(result);
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<OperationCost, Error> Db::DeleteAux(const Bytes& key, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

    auto result =
        grovedb_cxx::grovedb_delete_aux_with_tx(*m_impl->m_db, key_slice, *txn.m_impl->m_tx);
    return convert_cost(result);
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

// ---------------------------------------------------------------------------
// FindSubtrees
// ---------------------------------------------------------------------------

Result<Costed<std::vector<Path>>, Error> Db::FindSubtrees(const Path& path)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};

    auto result = grovedb_cxx::grovedb_find_subtrees(*m_impl->m_db, path_slice);
    auto cost = convert_cost(result.cost);

    // Decode wire-encoded paths: [u32 count][path₁]…
    wire::Reader r{{result.value.data(), result.value.size()}};
    auto count_r = r.U32();
    if (!count_r) {
      return Err(Error::Corruption("failed to decode find_subtrees count"));
    }
    if (*count_r > wire::MAX_VECTOR_SIZE) {
      return Err(Error::Corruption("find_subtrees count exceeds maximum"));
    }

    std::vector<Path> paths;
    paths.reserve(*count_r);
    for (uint32_t i{0}; i < *count_r; ++i) {
      auto p = wire::Read<Path>(r);
      if (!p) {
        return Err(Error::Corruption("failed to decode find_subtrees path"));
      }
      paths.push_back(std::move(*p));
    }
    return Costed<std::vector<Path>>{std::move(paths), cost};
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<Costed<std::vector<Path>>, Error> Db::FindSubtrees(const Path& path, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};

    auto result =
        grovedb_cxx::grovedb_find_subtrees_with_tx(*m_impl->m_db, path_slice, *txn.m_impl->m_tx);
    auto cost = convert_cost(result.cost);

    wire::Reader r{{result.value.data(), result.value.size()}};
    auto count_r = r.U32();
    if (!count_r) {
      return Err(Error::Corruption("failed to decode find_subtrees count"));
    }
    if (*count_r > wire::MAX_VECTOR_SIZE) {
      return Err(Error::Corruption("find_subtrees count exceeds maximum"));
    }

    std::vector<Path> paths;
    paths.reserve(*count_r);
    for (uint32_t i{0}; i < *count_r; ++i) {
      auto p = wire::Read<Path>(r);
      if (!p) {
        return Err(Error::Corruption("failed to decode find_subtrees path"));
      }
      paths.push_back(std::move(*p));
    }
    return Costed<std::vector<Path>>{std::move(paths), cost};
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}
} // namespace grovedb

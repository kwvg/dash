// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <types/db.h>
#include <types/transaction.h>
#include <util/ffi.h>

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
  return CallFFI([&]() -> Result<OperationCost, Error> {
    auto result = grovedb_cxx::grovedb_put_aux(**m_impl->m_db, ToSlice(key), ToSlice(value));
    return convert_cost(result);
  });
}

Result<OperationCost, Error>
Db::PutAux(const Bytes& key, const Bytes& value, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<OperationCost, Error> {
    auto result = grovedb_cxx::grovedb_put_aux_with_tx(
        **m_impl->m_db, ToSlice(key), ToSlice(value), *txn.m_impl->m_tx
    );
    return convert_cost(result);
  });
}

// ---------------------------------------------------------------------------
// GetAux
// ---------------------------------------------------------------------------

Result<Costed<std::optional<Bytes>>, Error> Db::GetAux(const Bytes& key)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Costed<std::optional<Bytes>>, Error> {
    auto result = grovedb_cxx::grovedb_get_aux(**m_impl->m_db, ToSlice(key));
    return ConvertOptionalBytes(result);
  });
}

Result<Costed<std::optional<Bytes>>, Error> Db::GetAux(const Bytes& key, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Costed<std::optional<Bytes>>, Error> {
    auto result =
        grovedb_cxx::grovedb_get_aux_with_tx(**m_impl->m_db, ToSlice(key), *txn.m_impl->m_tx);
    return ConvertOptionalBytes(result);
  });
}

// ---------------------------------------------------------------------------
// DeleteAux
// ---------------------------------------------------------------------------

Result<OperationCost, Error> Db::DeleteAux(const Bytes& key)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<OperationCost, Error> {
    auto result = grovedb_cxx::grovedb_delete_aux(**m_impl->m_db, ToSlice(key));
    return convert_cost(result);
  });
}

Result<OperationCost, Error> Db::DeleteAux(const Bytes& key, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<OperationCost, Error> {
    auto result =
        grovedb_cxx::grovedb_delete_aux_with_tx(**m_impl->m_db, ToSlice(key), *txn.m_impl->m_tx);
    return convert_cost(result);
  });
}

// ---------------------------------------------------------------------------
// FindSubtrees
// ---------------------------------------------------------------------------

Result<Costed<std::vector<Path>>, Error> Db::FindSubtrees(const Path& path)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Costed<std::vector<Path>>, Error> {
    auto path_buf = wire::Encode(path);
    auto result = grovedb_cxx::grovedb_find_subtrees(**m_impl->m_db, ToSlice(path_buf));
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
  });
}

Result<Costed<std::vector<Path>>, Error> Db::FindSubtrees(const Path& path, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Costed<std::vector<Path>>, Error> {
    auto path_buf = wire::Encode(path);
    auto result = grovedb_cxx::grovedb_find_subtrees_with_tx(
        **m_impl->m_db, ToSlice(path_buf), *txn.m_impl->m_tx
    );
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
  });
}
} // namespace grovedb

// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <types/db.h>
#include <types/transaction.h>
#include <util/ffi.h>

#include <grovedb/wire.h>

namespace grovedb {
// ---------------------------------------------------------------------------
// Get — follows references
// ---------------------------------------------------------------------------

Result<Costed<Element>, Error> Db::Get(const Path& path, const Bytes& key)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Costed<Element>, Error> {
    auto path_buf = wire::Encode(path);
    auto result = grovedb_cxx::grovedb_get(**m_impl->m_db, ToSlice(path_buf), ToSlice(key));
    return ConvertElement(result);
  });
}

Result<Costed<Element>, Error> Db::Get(const Path& path, const Bytes& key, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Costed<Element>, Error> {
    auto path_buf = wire::Encode(path);
    auto result = grovedb_cxx::grovedb_get_with_tx(
        **m_impl->m_db, ToSlice(path_buf), ToSlice(key), *txn.m_impl->m_tx
    );
    return ConvertElement(result);
  });
}

// ---------------------------------------------------------------------------
// GetDirect — no reference following
// ---------------------------------------------------------------------------

Result<Costed<Element>, Error> Db::GetDirect(const Path& path, const Bytes& key)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Costed<Element>, Error> {
    auto path_buf = wire::Encode(path);
    auto result = grovedb_cxx::grovedb_get_raw(**m_impl->m_db, ToSlice(path_buf), ToSlice(key));
    return ConvertElement(result);
  });
}

Result<Costed<Element>, Error>
Db::GetDirect(const Path& path, const Bytes& key, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Costed<Element>, Error> {
    auto path_buf = wire::Encode(path);
    auto result = grovedb_cxx::grovedb_get_raw_with_tx(
        **m_impl->m_db, ToSlice(path_buf), ToSlice(key), *txn.m_impl->m_tx
    );
    return ConvertElement(result);
  });
}

// ---------------------------------------------------------------------------
// GetOptional — returns nullopt when missing
// ---------------------------------------------------------------------------

Result<Costed<std::optional<Element>>, Error> Db::GetOptional(const Path& path, const Bytes& key)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Costed<std::optional<Element>>, Error> {
    auto path_buf = wire::Encode(path);
    auto result =
        grovedb_cxx::grovedb_get_raw_optional(**m_impl->m_db, ToSlice(path_buf), ToSlice(key));
    return ConvertOptionalElement(result);
  });
}

Result<Costed<std::optional<Element>>, Error>
Db::GetOptional(const Path& path, const Bytes& key, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Costed<std::optional<Element>>, Error> {
    auto path_buf = wire::Encode(path);
    auto result = grovedb_cxx::grovedb_get_raw_optional_with_tx(
        **m_impl->m_db, ToSlice(path_buf), ToSlice(key), *txn.m_impl->m_tx
    );
    return ConvertOptionalElement(result);
  });
}

// ---------------------------------------------------------------------------
// KeyExists — existence check
// ---------------------------------------------------------------------------

Result<Costed<bool>, Error> Db::KeyExists(const Path& path, const Bytes& key)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Costed<bool>, Error> {
    auto path_buf = wire::Encode(path);
    auto result = grovedb_cxx::grovedb_has_raw(**m_impl->m_db, ToSlice(path_buf), ToSlice(key));
    return ConvertBool(result);
  });
}

Result<Costed<bool>, Error>
Db::KeyExists(const Path& path, const Bytes& key, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Costed<bool>, Error> {
    auto path_buf = wire::Encode(path);
    auto result = grovedb_cxx::grovedb_has_raw_with_tx(
        **m_impl->m_db, ToSlice(path_buf), ToSlice(key), *txn.m_impl->m_tx
    );
    return ConvertBool(result);
  });
}

// ---------------------------------------------------------------------------
// SubtreeExists — subtree/path validation
// ---------------------------------------------------------------------------

Result<Costed<bool>, Error> Db::SubtreeExists(const Path& path)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Costed<bool>, Error> {
    auto path_buf = wire::Encode(path);
    auto result = grovedb_cxx::grovedb_check_subtree_exists(**m_impl->m_db, ToSlice(path_buf));
    return ConvertBool(result);
  });
}

Result<Costed<bool>, Error> Db::SubtreeExists(const Path& path, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Costed<bool>, Error> {
    auto path_buf = wire::Encode(path);
    auto result = grovedb_cxx::grovedb_check_subtree_exists_with_tx(
        **m_impl->m_db, ToSlice(path_buf), *txn.m_impl->m_tx
    );
    return ConvertBool(result);
  });
}

// ---------------------------------------------------------------------------
// IsEmptyTree
// ---------------------------------------------------------------------------

Result<Costed<bool>, Error> Db::IsEmptyTree(const Path& path)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Costed<bool>, Error> {
    auto path_buf = wire::Encode(path);
    auto result = grovedb_cxx::grovedb_is_empty_tree(**m_impl->m_db, ToSlice(path_buf));
    return ConvertBool(result);
  });
}

Result<Costed<bool>, Error> Db::IsEmptyTree(const Path& path, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Costed<bool>, Error> {
    auto path_buf = wire::Encode(path);
    auto result = grovedb_cxx::grovedb_is_empty_tree_with_tx(
        **m_impl->m_db, ToSlice(path_buf), *txn.m_impl->m_tx
    );
    return ConvertBool(result);
  });
}
} // namespace grovedb

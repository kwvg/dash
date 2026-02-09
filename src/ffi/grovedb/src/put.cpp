// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <db_internal.h>
#include <types/transaction.h>
#include <util/ffi.h>

#include <grovedb/wire.h>

namespace grovedb {
// ---------------------------------------------------------------------------
// Put — unconditional insert
// ---------------------------------------------------------------------------

Result<OperationCost, Error> Db::Put(const Path& path, const Bytes& key, const Element& element)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<OperationCost, Error> {
    auto path_buf = wire::Encode(path);
    auto result = grovedb_cxx::grovedb_insert(
        *m_impl->m_db, ToSlice(path_buf), ToSlice(key), ToSlice(element.m_data)
    );
    return convert_cost(result);
  });
}

Result<OperationCost, Error>
Db::Put(const Path& path, const Bytes& key, const Element& element, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<OperationCost, Error> {
    auto path_buf = wire::Encode(path);
    auto result = grovedb_cxx::grovedb_insert_with_tx(
        *m_impl->m_db, ToSlice(path_buf), ToSlice(key), ToSlice(element.m_data), *txn.m_impl->m_tx
    );
    return convert_cost(result);
  });
}

// ---------------------------------------------------------------------------
// PutIfAbsent
// ---------------------------------------------------------------------------

Result<Costed<bool>, Error>
Db::PutIfAbsent(const Path& path, const Bytes& key, const Element& element)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Costed<bool>, Error> {
    auto path_buf = wire::Encode(path);
    auto result = grovedb_cxx::grovedb_insert_if_not_exists(
        *m_impl->m_db, ToSlice(path_buf), ToSlice(key), ToSlice(element.m_data)
    );
    return ConvertBool(result);
  });
}

Result<Costed<bool>, Error>
Db::PutIfAbsent(const Path& path, const Bytes& key, const Element& element, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Costed<bool>, Error> {
    auto path_buf = wire::Encode(path);
    auto result = grovedb_cxx::grovedb_insert_if_not_exists_with_tx(
        *m_impl->m_db, ToSlice(path_buf), ToSlice(key), ToSlice(element.m_data), *txn.m_impl->m_tx
    );
    return ConvertBool(result);
  });
}

// ---------------------------------------------------------------------------
// PutIfAbsentAndGet
// ---------------------------------------------------------------------------

Result<Costed<std::optional<Element>>, Error>
Db::PutIfAbsentAndGet(const Path& path, const Bytes& key, const Element& element)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Costed<std::optional<Element>>, Error> {
    auto path_buf = wire::Encode(path);
    auto result = grovedb_cxx::grovedb_insert_if_not_exists_return_existing(
        *m_impl->m_db, ToSlice(path_buf), ToSlice(key), ToSlice(element.m_data)
    );
    return ConvertOptionalElement(result);
  });
}

Result<Costed<std::optional<Element>>, Error> Db::PutIfAbsentAndGet(
    const Path& path, const Bytes& key, const Element& element, const Transaction& txn
)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Costed<std::optional<Element>>, Error> {
    auto path_buf = wire::Encode(path);
    auto result = grovedb_cxx::grovedb_insert_if_not_exists_return_existing_with_tx(
        *m_impl->m_db, ToSlice(path_buf), ToSlice(key), ToSlice(element.m_data), *txn.m_impl->m_tx
    );
    return ConvertOptionalElement(result);
  });
}

// ---------------------------------------------------------------------------
// PutIfChanged
// ---------------------------------------------------------------------------

Result<Costed<Db::ChangedValue>, Error>
Db::PutIfChanged(const Path& path, const Bytes& key, const Element& element)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Costed<ChangedValue>, Error> {
    auto path_buf = wire::Encode(path);
    auto result = grovedb_cxx::grovedb_insert_if_changed_value(
        *m_impl->m_db, ToSlice(path_buf), ToSlice(key), ToSlice(element.m_data)
    );
    return ConvertChangedValue(result);
  });
}

Result<Costed<Db::ChangedValue>, Error>
Db::PutIfChanged(const Path& path, const Bytes& key, const Element& element, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  return CallFFI([&]() -> Result<Costed<ChangedValue>, Error> {
    auto path_buf = wire::Encode(path);
    auto result = grovedb_cxx::grovedb_insert_if_changed_value_with_tx(
        *m_impl->m_db, ToSlice(path_buf), ToSlice(key), ToSlice(element.m_data), *txn.m_impl->m_tx
    );
    return ConvertChangedValue(result);
  });
}
} // namespace grovedb

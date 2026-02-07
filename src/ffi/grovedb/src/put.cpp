// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <db_internal.h>
#include <types/transaction.h>

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
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};
    rust::Slice<const uint8_t> elem_slice{element.m_data.data(), element.m_data.size()};

    auto result = grovedb_cxx::grovedb_insert(*m_impl->m_db, path_slice, key_slice, elem_slice);
    return convert_cost(result);
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<OperationCost, Error>
Db::Put(const Path& path, const Bytes& key, const Element& element, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};
    rust::Slice<const uint8_t> elem_slice{element.m_data.data(), element.m_data.size()};

    auto result = grovedb_cxx::grovedb_insert_with_tx(
        *m_impl->m_db, path_slice, key_slice, elem_slice, *txn.m_impl->m_tx
    );
    return convert_cost(result);
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
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
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};
    rust::Slice<const uint8_t> elem_slice{element.m_data.data(), element.m_data.size()};

    auto result =
        grovedb_cxx::grovedb_insert_if_not_exists(*m_impl->m_db, path_slice, key_slice, elem_slice);
    return Costed<bool>{
        .m_value = result.value,
        .m_cost = convert_cost(result.cost),
    };
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<Costed<bool>, Error>
Db::PutIfAbsent(const Path& path, const Bytes& key, const Element& element, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};
    rust::Slice<const uint8_t> elem_slice{element.m_data.data(), element.m_data.size()};

    auto result = grovedb_cxx::grovedb_insert_if_not_exists_with_tx(
        *m_impl->m_db, path_slice, key_slice, elem_slice, *txn.m_impl->m_tx
    );
    return Costed<bool>{
        .m_value = result.value,
        .m_cost = convert_cost(result.cost),
    };
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
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
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};
    rust::Slice<const uint8_t> elem_slice{element.m_data.data(), element.m_data.size()};

    auto result = grovedb_cxx::grovedb_insert_if_not_exists_return_existing(
        *m_impl->m_db, path_slice, key_slice, elem_slice
    );
    auto cost = convert_cost(result.cost);
    if (result.has_element) {
      Element existing;
      existing.m_data.assign(result.element.begin(), result.element.end());
      return Costed<std::optional<Element>>{
          .m_value = std::move(existing),
          .m_cost = cost,
      };
    }
    return Costed<std::optional<Element>>{
        .m_value = std::nullopt,
        .m_cost = cost,
    };
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<Costed<std::optional<Element>>, Error> Db::PutIfAbsentAndGet(
    const Path& path, const Bytes& key, const Element& element, const Transaction& txn
)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};
    rust::Slice<const uint8_t> elem_slice{element.m_data.data(), element.m_data.size()};

    auto result = grovedb_cxx::grovedb_insert_if_not_exists_return_existing_with_tx(
        *m_impl->m_db, path_slice, key_slice, elem_slice, *txn.m_impl->m_tx
    );
    auto cost = convert_cost(result.cost);
    if (result.has_element) {
      Element existing;
      existing.m_data.assign(result.element.begin(), result.element.end());
      return Costed<std::optional<Element>>{
          .m_value = std::move(existing),
          .m_cost = cost,
      };
    }
    return Costed<std::optional<Element>>{
        .m_value = std::nullopt,
        .m_cost = cost,
    };
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
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
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};
    rust::Slice<const uint8_t> elem_slice{element.m_data.data(), element.m_data.size()};

    auto result = grovedb_cxx::grovedb_insert_if_changed_value(
        *m_impl->m_db, path_slice, key_slice, elem_slice
    );
    auto cost = convert_cost(result.cost);
    std::optional<Element> prev;
    if (result.has_previous_element) {
      Element elem;
      elem.m_data.assign(result.previous_element.begin(), result.previous_element.end());
      prev = std::move(elem);
    }
    return Costed<ChangedValue>{
        .m_value = ChangedValue{.m_changed = result.changed, .m_previous = std::move(prev)},
        .m_cost = cost,
    };
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<Costed<Db::ChangedValue>, Error>
Db::PutIfChanged(const Path& path, const Bytes& key, const Element& element, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};
    rust::Slice<const uint8_t> elem_slice{element.m_data.data(), element.m_data.size()};

    auto result = grovedb_cxx::grovedb_insert_if_changed_value_with_tx(
        *m_impl->m_db, path_slice, key_slice, elem_slice, *txn.m_impl->m_tx
    );
    auto cost = convert_cost(result.cost);
    std::optional<Element> prev;
    if (result.has_previous_element) {
      Element elem;
      elem.m_data.assign(result.previous_element.begin(), result.previous_element.end());
      prev = std::move(elem);
    }
    return Costed<ChangedValue>{
        .m_value = ChangedValue{.m_changed = result.changed, .m_previous = std::move(prev)},
        .m_cost = cost,
    };
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}
} // namespace grovedb

// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <db_internal.h>
#include <types/transaction.h>

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
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

    auto result = grovedb_cxx::grovedb_get(*m_impl->m_db, path_slice, key_slice);
    Element elem;
    elem.m_data.assign(result.element.begin(), result.element.end());
    return Costed<Element>{
        .m_value = std::move(elem),
        .m_cost = convert_cost(result.cost),
    };
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<Costed<Element>, Error> Db::Get(const Path& path, const Bytes& key, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

    auto result =
        grovedb_cxx::grovedb_get_with_tx(*m_impl->m_db, path_slice, key_slice, *txn.m_impl->m_tx);
    Element elem;
    elem.m_data.assign(result.element.begin(), result.element.end());
    return Costed<Element>{
        .m_value = std::move(elem),
        .m_cost = convert_cost(result.cost),
    };
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

// ---------------------------------------------------------------------------
// GetDirect — no reference following
// ---------------------------------------------------------------------------

Result<Costed<Element>, Error> Db::GetDirect(const Path& path, const Bytes& key)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

    auto result = grovedb_cxx::grovedb_get_raw(*m_impl->m_db, path_slice, key_slice);
    Element elem;
    elem.m_data.assign(result.element.begin(), result.element.end());
    return Costed<Element>{
        .m_value = std::move(elem),
        .m_cost = convert_cost(result.cost),
    };
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<Costed<Element>, Error>
Db::GetDirect(const Path& path, const Bytes& key, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

    auto result = grovedb_cxx::grovedb_get_raw_with_tx(
        *m_impl->m_db, path_slice, key_slice, *txn.m_impl->m_tx
    );
    Element elem;
    elem.m_data.assign(result.element.begin(), result.element.end());
    return Costed<Element>{
        .m_value = std::move(elem),
        .m_cost = convert_cost(result.cost),
    };
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

// ---------------------------------------------------------------------------
// GetOptional — returns nullopt when missing
// ---------------------------------------------------------------------------

Result<Costed<std::optional<Element>>, Error> Db::GetOptional(const Path& path, const Bytes& key)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

    auto result = grovedb_cxx::grovedb_get_raw_optional(*m_impl->m_db, path_slice, key_slice);
    auto cost = convert_cost(result.cost);
    if (result.has_element) {
      Element elem;
      elem.m_data.assign(result.element.begin(), result.element.end());
      return Costed<std::optional<Element>>{
          .m_value = std::move(elem),
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

Result<Costed<std::optional<Element>>, Error>
Db::GetOptional(const Path& path, const Bytes& key, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

    auto result = grovedb_cxx::grovedb_get_raw_optional_with_tx(
        *m_impl->m_db, path_slice, key_slice, *txn.m_impl->m_tx
    );
    auto cost = convert_cost(result.cost);
    if (result.has_element) {
      Element elem;
      elem.m_data.assign(result.element.begin(), result.element.end());
      return Costed<std::optional<Element>>{
          .m_value = std::move(elem),
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
// KeyExists — existence check
// ---------------------------------------------------------------------------

Result<Costed<bool>, Error> Db::KeyExists(const Path& path, const Bytes& key)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

    auto result = grovedb_cxx::grovedb_has_raw(*m_impl->m_db, path_slice, key_slice);
    return Costed<bool>{
        .m_value = result.value,
        .m_cost = convert_cost(result.cost),
    };
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<Costed<bool>, Error>
Db::KeyExists(const Path& path, const Bytes& key, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};
    rust::Slice<const uint8_t> key_slice{key.data(), key.size()};

    auto result = grovedb_cxx::grovedb_has_raw_with_tx(
        *m_impl->m_db, path_slice, key_slice, *txn.m_impl->m_tx
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
// SubtreeExists — subtree/path validation
// ---------------------------------------------------------------------------

Result<Costed<bool>, Error> Db::SubtreeExists(const Path& path)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};

    auto result = grovedb_cxx::grovedb_check_subtree_exists(*m_impl->m_db, path_slice);
    return Costed<bool>{
        .m_value = result.value,
        .m_cost = convert_cost(result.cost),
    };
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<Costed<bool>, Error> Db::SubtreeExists(const Path& path, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};

    auto result = grovedb_cxx::grovedb_check_subtree_exists_with_tx(
        *m_impl->m_db, path_slice, *txn.m_impl->m_tx
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
// IsEmptyTree
// ---------------------------------------------------------------------------

Result<Costed<bool>, Error> Db::IsEmptyTree(const Path& path)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};

    auto result = grovedb_cxx::grovedb_is_empty_tree(*m_impl->m_db, path_slice);
    return Costed<bool>{
        .m_value = result.value,
        .m_cost = convert_cost(result.cost),
    };
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<Costed<bool>, Error> Db::IsEmptyTree(const Path& path, const Transaction& txn)
{
  if (!m_impl) {
    return Err(Error::InvalidArgument("called on uninitialized database"));
  }
  try {
    auto path_buf = wire::Encode(path);
    rust::Slice<const uint8_t> path_slice{path_buf.data(), path_buf.size()};

    auto result =
        grovedb_cxx::grovedb_is_empty_tree_with_tx(*m_impl->m_db, path_slice, *txn.m_impl->m_tx);
    return Costed<bool>{
        .m_value = result.value,
        .m_cost = convert_cost(result.cost),
    };
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}
} // namespace grovedb

// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GROVEDB_ELEMENT_H
#define GROVEDB_ELEMENT_H

#include <grovedb/status.h>
#include <grovedb/types.h>

namespace grovedb {
class Db;

/**
 * Opaque container for a GroveDB element in its serialized (bincode) form.
 *
 * Populated by Db::Get / Db::GetDirect / Db::GetOptional, or constructed
 * with one of the static factory methods.  The raw bytes can be inspected
 * via data() and are accepted by Db::Put and related methods.
 */
class Element
{
public:
    constexpr Element() = default;

    // -- Factories --------------------------------------------------------

    /**
     * Create an item element containing arbitrary value bytes.
     *
     * @param[in]  value    Raw value payload.
     * @param[out] element  Receives the serialized element on success.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    static Status Item(const Bytes& value, Element& element);

    /**
     * Create an empty subtree element.
     *
     * @param[out] element  Receives the serialized element on success.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    static Status EmptyTree(Element& element);

    /**
     * Create an empty sum tree element.
     *
     * @param[out] element  Receives the serialized element on success.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    static Status EmptySumTree(Element& element);

    /**
     * Create a sum item element with the given value.
     *
     * @param[in]  value    The sum value.
     * @param[out] element  Receives the serialized element on success.
     * @return Status::Ok() on success; an error Status otherwise.
     */
    static Status SumItem(int64_t value, Element& element);

    // -- Accessors --------------------------------------------------------

    /** Access the raw serialized representation. */
    const Bytes& data() const { return m_data; }

    /** True when this element holds no data (default-constructed). */
    constexpr bool empty() const { return m_data.empty(); }

private:
    friend class Db;
    Bytes m_data;
};

} // namespace grovedb

#endif // GROVEDB_ELEMENT_H

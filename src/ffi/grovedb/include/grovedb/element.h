// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GROVEDB_ELEMENT_H
#define GROVEDB_ELEMENT_H

#include <grovedb/types.h>

namespace grovedb {
class Db;

/**
 * Opaque container for a GroveDB element in its serialized (bincode) form.
 *
 * Populated by Db::Get / Db::GetDirect / Db::GetOptional.  The raw bytes
 * can be inspected via data() and will be accepted by future Db::Put
 * methods for round-tripping without re-serialization.
 */
class Element
{
public:
    constexpr Element() = default;

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

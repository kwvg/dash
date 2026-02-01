// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GROVEDB_PROOF_H
#define GROVEDB_PROOF_H

#include <grovedb/element.h>
#include <grovedb/types.h>

#include <optional>

namespace grovedb {

/**
 * Options controlling proof generation behavior.
 */
struct ProveOptions {
    /**
     * Decrease available limit by 1 for empty subtree results.
     *
     * Generally should be true.  Only set false when the query structure
     * is known to have few empty subtrees; setting false when many empty
     * subtrees exist can exhaust memory.
     */
    bool m_decrease_limit_on_empty{true};
};

/**
 * Options controlling proof verification behavior.
 */
struct VerifyOptions {
    /** Return absence proofs for key-based query items that don't exist. */
    bool m_absence_proofs{true};
    /** Verify that the proof contains no extraneous data. */
    bool m_verify_succinctness{true};
    /** Include empty trees in verification results. */
    bool m_include_empty_trees{false};
};

/**
 * Single entry from a proof verification result.
 *
 * Each entry corresponds to one path+key in the query, with an optional
 * element (absent when the key was not found, or for absence proofs).
 */
struct ProofResultEntry {
    Path m_path;
    Bytes m_key;
    std::optional<Element> m_element;
};

} // namespace grovedb

#endif // GROVEDB_PROOF_H

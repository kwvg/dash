// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifdef HAVE_GROVEDB_CONFIG
#include <config/grovedb-config.h>
#endif

#include "db_internal.h"

#include <grovedb/wire.h>

#include <types/query.h>

#include <span>

namespace grovedb {

// ---------------------------------------------------------------------------
// Proof operations
// ---------------------------------------------------------------------------

Status Db::Prove(const PathQuery& query, Bytes& proof, OperationCost& cost)
{
    return Prove(query, ProveOptions{}, proof, cost);
}

Status Db::Prove(const PathQuery& query, const ProveOptions& options,
                 Bytes& proof, OperationCost& cost)
{
    try {
        auto result = grovedb_cxx::grovedb_prove_query(
            *m_impl->m_db, *query.m_impl->m_query,
            options.m_decrease_limit_on_empty);
        proof.assign(result.proof.begin(), result.proof.end());
        cost = convert_cost(result.cost);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::DecodeVerifyResult(
    std::span<const uint8_t> root_hash_bytes,
    std::span<const uint8_t> entries_bytes,
    Hash& root_hash,
    std::vector<ProofResultEntry>& entries)
{
    if (root_hash_bytes.size() != 32) {
        return Status::Corruption("proof: root hash is not 32 bytes");
    }
    std::copy(root_hash_bytes.begin(), root_hash_bytes.end(), root_hash.begin());

    wire::Reader r{entries_bytes};
    uint32_t count{0};
    if (auto s = r.U32(count); !s.ok()) return s;

    entries.clear();
    entries.reserve(count);
    for (uint32_t i{0}; i < count; ++i) {
        ProofResultEntry entry;
        if (auto s = wire::WireRead(r, entry.m_path); !s.ok()) return s;
        if (auto s = r.Bytes(entry.m_key); !s.ok()) return s;

        uint8_t has_elem{0};
        if (auto s = r.U8(has_elem); !s.ok()) return s;
        if (has_elem) {
            Bytes elem_bytes;
            if (auto s = r.Bytes(elem_bytes); !s.ok()) return s;
            Element elem;
            elem.m_data = std::move(elem_bytes);
            entry.m_element = std::move(elem);
        }

        entries.push_back(std::move(entry));
    }
    return Status::Ok();
}

Status Db::VerifyQuery(
    const Bytes& proof, const PathQuery& query,
    Hash& root_hash, std::vector<ProofResultEntry>& entries)
{
    try {
        rust::Slice<const uint8_t> proof_slice{proof.data(), proof.size()};
        auto result = grovedb_cxx::grovedb_verify_query(
            proof_slice, *query.m_impl->m_query);
        return DecodeVerifyResult(
            {result.root_hash.data(), result.root_hash.size()},
            {result.entries.data(), result.entries.size()},
            root_hash, entries);
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::VerifyQuery(
    const Bytes& proof, const PathQuery& query,
    const VerifyOptions& options,
    Hash& root_hash, std::vector<ProofResultEntry>& entries)
{
    try {
        rust::Slice<const uint8_t> proof_slice{proof.data(), proof.size()};
        auto result = grovedb_cxx::grovedb_verify_query_with_options(
            proof_slice, *query.m_impl->m_query,
            options.m_absence_proofs,
            options.m_verify_succinctness,
            options.m_include_empty_trees);
        return DecodeVerifyResult(
            {result.root_hash.data(), result.root_hash.size()},
            {result.entries.data(), result.entries.size()},
            root_hash, entries);
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::VerifySubsetQuery(
    const Bytes& proof, const PathQuery& query,
    Hash& root_hash, std::vector<ProofResultEntry>& entries)
{
    try {
        rust::Slice<const uint8_t> proof_slice{proof.data(), proof.size()};
        auto result = grovedb_cxx::grovedb_verify_subset_query(
            proof_slice, *query.m_impl->m_query);
        return DecodeVerifyResult(
            {result.root_hash.data(), result.root_hash.size()},
            {result.entries.data(), result.entries.size()},
            root_hash, entries);
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::VerifyQueryWithAbsenceProof(
    const Bytes& proof, const PathQuery& query,
    Hash& root_hash, std::vector<ProofResultEntry>& entries)
{
    try {
        rust::Slice<const uint8_t> proof_slice{proof.data(), proof.size()};
        auto result = grovedb_cxx::grovedb_verify_query_with_absence_proof(
            proof_slice, *query.m_impl->m_query);
        return DecodeVerifyResult(
            {result.root_hash.data(), result.root_hash.size()},
            {result.entries.data(), result.entries.size()},
            root_hash, entries);
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

Status Db::VerifySubsetQueryWithAbsenceProof(
    const Bytes& proof, const PathQuery& query,
    Hash& root_hash, std::vector<ProofResultEntry>& entries)
{
    try {
        rust::Slice<const uint8_t> proof_slice{proof.data(), proof.size()};
        auto result = grovedb_cxx::grovedb_verify_subset_query_with_absence_proof(
            proof_slice, *query.m_impl->m_query);
        return DecodeVerifyResult(
            {result.root_hash.data(), result.root_hash.size()},
            {result.entries.data(), result.entries.size()},
            root_hash, entries);
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

} // namespace grovedb

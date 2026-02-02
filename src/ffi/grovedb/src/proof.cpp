// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifdef HAVE_GROVEDB_CONFIG
#include <config/grovedb-config.h>
#endif

#include "db_internal.h"

#include <grovedb/wire.h>

#include <types/query.h>

#include <util/assert.h>

#include <span>
#include <vector>

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
    Assert(m_impl, "called on uninitialized database");
    Assert(query.m_impl, "called with uninitialized query");
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
    Assert(query.m_impl, "called with uninitialized query");
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
    Assert(query.m_impl, "called with uninitialized query");
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
    Assert(query.m_impl, "called with uninitialized query");
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
    Assert(query.m_impl, "called with uninitialized query");
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
    Assert(query.m_impl, "called with uninitialized query");
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

// ---------------------------------------------------------------------------
// Chained query verification
// ---------------------------------------------------------------------------

Status Db::DecodeChainedVerifyResult(
    std::span<const uint8_t> root_hash_bytes,
    std::span<const uint8_t> result_sets_bytes,
    Hash& root_hash,
    std::vector<std::vector<ProofResultEntry>>& all_results)
{
    if (root_hash_bytes.size() != 32) {
        return Status::Corruption("proof: root hash is not 32 bytes");
    }
    std::copy(root_hash_bytes.begin(), root_hash_bytes.end(), root_hash.begin());

    wire::Reader r{result_sets_bytes};
    uint32_t set_count{0};
    if (auto s = r.U32(set_count); !s.ok()) return s;

    all_results.clear();
    all_results.reserve(set_count);
    for (uint32_t si{0}; si < set_count; ++si) {
        uint32_t entry_count{0};
        if (auto s = r.U32(entry_count); !s.ok()) return s;

        std::vector<ProofResultEntry> result_set;
        result_set.reserve(entry_count);
        for (uint32_t ei{0}; ei < entry_count; ++ei) {
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

            result_set.push_back(std::move(entry));
        }
        all_results.push_back(std::move(result_set));
    }
    return Status::Ok();
}

Status Db::VerifyChainedQueries(
    const Bytes& proof,
    const PathQuery& first_query,
    const std::vector<const PathQuery*>& chained_queries,
    Hash& root_hash,
    std::vector<std::vector<ProofResultEntry>>& all_results)
{
    Assert(first_query.m_impl, "called with uninitialized query");
    try {
        // Build the PathQueryVec accumulator.
        auto vec = grovedb_cxx::grovedb_path_query_vec_new();
        for (const auto* q : chained_queries) {
            Assert(q != nullptr, "chained_queries must not contain null pointers");
            Assert(q->m_impl != nullptr, "called with uninitialized query");
            if (!q || !q->m_impl) {
                return Status::InvalidArgument("chained_queries contains invalid query");
            }
            grovedb_cxx::grovedb_path_query_vec_push(*vec, *q->m_impl->m_query);
        }

        rust::Slice<const uint8_t> proof_slice{proof.data(), proof.size()};
        auto result = grovedb_cxx::grovedb_verify_chained_queries(
            proof_slice, *first_query.m_impl->m_query, *vec);

        return DecodeChainedVerifyResult(
            {result.root_hash.data(), result.root_hash.size()},
            {result.result_sets.data(), result.result_sets.size()},
            root_hash, all_results);
    } catch (const std::exception& e) {
        return Status::IOError(e.what());
    }
}

} // namespace grovedb

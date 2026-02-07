// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <db_internal.h>
#include <decode_internal.h>
#include <types/query.h>

#include <grovedb/query.h>

#include <rust/grovedb_cxx/lib.h>

#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace grovedb {
namespace {
/**
 * Decode verify result wire data into PathKeyElement entries.
 *
 * Reuses the shared DecodePathKeyElements from decode_internal.h.
 */
Result<std::vector<PathKeyElement>, Error> DecodeVerifyResults(std::span<const uint8_t> data)
{
  return DecodePathKeyElements(data).transform_error([](wire::Error) {
    return Error::Corruption("failed to decode proof verify results");
  });
}
} // anonymous namespace

// Type aliases for brevity (nested structs require full qualification
// in trailing return types).
using PVR = Db::ProofVerifyResult;
using CVR = Db::ChainedVerifyResult;

// ---------------------------------------------------------------------------
// Prove
// ---------------------------------------------------------------------------

Result<Costed<Bytes>, Error> Db::Prove(const PathQuery& query, bool decrease_limit_on_empty)
{
  try {
    auto result = grovedb_cxx::grovedb_prove_query(
        *m_impl->m_db, *query.m_impl->m_query, decrease_limit_on_empty
    );
    Bytes proof{result.proof.begin(), result.proof.end()};
    return Costed<Bytes>{std::move(proof), convert_cost(result.cost)};
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

// ---------------------------------------------------------------------------
// Verify
// ---------------------------------------------------------------------------

Result<Costed<PVR>, Error> Db::VerifyQuery(const Bytes& proof, const PathQuery& query)
{
  try {
    rust::Slice<const uint8_t> proof_slice{proof.data(), proof.size()};
    auto result = grovedb_cxx::grovedb_verify_query(proof_slice, *query.m_impl->m_query);
    if (result.root_hash.size() != 32) {
      return Err(
          Error::Corruption(
              "root hash: expected 32 bytes, got " + std::to_string(result.root_hash.size())
          )
      );
    }
    Hash root_hash{};
    std::copy(result.root_hash.begin(), result.root_hash.end(), root_hash.begin());
    return DecodeVerifyResults({result.results.data(), result.results.size()})
        .transform([&](std::vector<PathKeyElement> entries) {
          return Costed<PVR>{PVR{root_hash, std::move(entries)}, convert_cost(result.cost)};
        });
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<Costed<PVR>, Error> Db::VerifyQueryWithOptions(
    const Bytes& proof,
    const PathQuery& query,
    bool absence_proofs,
    bool verify_succinctness,
    bool include_empty_trees
)
{
  try {
    rust::Slice<const uint8_t> proof_slice{proof.data(), proof.size()};
    auto result = grovedb_cxx::grovedb_verify_query_with_options(
        proof_slice,
        *query.m_impl->m_query,
        absence_proofs,
        verify_succinctness,
        include_empty_trees
    );
    if (result.root_hash.size() != 32) {
      return Err(
          Error::Corruption(
              "root hash: expected 32 bytes, got " + std::to_string(result.root_hash.size())
          )
      );
    }
    Hash root_hash{};
    std::copy(result.root_hash.begin(), result.root_hash.end(), root_hash.begin());
    return DecodeVerifyResults({result.results.data(), result.results.size()})
        .transform([&](std::vector<PathKeyElement> entries) {
          return Costed<PVR>{PVR{root_hash, std::move(entries)}, convert_cost(result.cost)};
        });
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<Costed<PVR>, Error> Db::VerifySubsetQuery(const Bytes& proof, const PathQuery& query)
{
  try {
    rust::Slice<const uint8_t> proof_slice{proof.data(), proof.size()};
    auto result = grovedb_cxx::grovedb_verify_subset_query(proof_slice, *query.m_impl->m_query);
    if (result.root_hash.size() != 32) {
      return Err(
          Error::Corruption(
              "root hash: expected 32 bytes, got " + std::to_string(result.root_hash.size())
          )
      );
    }
    Hash root_hash{};
    std::copy(result.root_hash.begin(), result.root_hash.end(), root_hash.begin());
    return DecodeVerifyResults({result.results.data(), result.results.size()})
        .transform([&](std::vector<PathKeyElement> entries) {
          return Costed<PVR>{PVR{root_hash, std::move(entries)}, convert_cost(result.cost)};
        });
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<Costed<PVR>, Error>
Db::VerifyQueryWithAbsenceProof(const Bytes& proof, const PathQuery& query)
{
  try {
    rust::Slice<const uint8_t> proof_slice{proof.data(), proof.size()};
    auto result =
        grovedb_cxx::grovedb_verify_query_with_absence_proof(proof_slice, *query.m_impl->m_query);
    if (result.root_hash.size() != 32) {
      return Err(
          Error::Corruption(
              "root hash: expected 32 bytes, got " + std::to_string(result.root_hash.size())
          )
      );
    }
    Hash root_hash{};
    std::copy(result.root_hash.begin(), result.root_hash.end(), root_hash.begin());
    return DecodeVerifyResults({result.results.data(), result.results.size()})
        .transform([&](std::vector<PathKeyElement> entries) {
          return Costed<PVR>{PVR{root_hash, std::move(entries)}, convert_cost(result.cost)};
        });
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

Result<Costed<PVR>, Error>
Db::VerifySubsetQueryWithAbsenceProof(const Bytes& proof, const PathQuery& query)
{
  try {
    rust::Slice<const uint8_t> proof_slice{proof.data(), proof.size()};
    auto result = grovedb_cxx::grovedb_verify_subset_query_with_absence_proof(
        proof_slice, *query.m_impl->m_query
    );
    if (result.root_hash.size() != 32) {
      return Err(
          Error::Corruption(
              "root hash: expected 32 bytes, got " + std::to_string(result.root_hash.size())
          )
      );
    }
    Hash root_hash{};
    std::copy(result.root_hash.begin(), result.root_hash.end(), root_hash.begin());
    return DecodeVerifyResults({result.results.data(), result.results.size()})
        .transform([&](std::vector<PathKeyElement> entries) {
          return Costed<PVR>{PVR{root_hash, std::move(entries)}, convert_cost(result.cost)};
        });
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}

// ---------------------------------------------------------------------------
// Chained verify
// ---------------------------------------------------------------------------

Result<Costed<CVR>, Error> Db::VerifyChainedQueries(
    const Bytes& proof, const PathQuery& first_query, const std::vector<PathQuery*>& chained_queries
)
{
  try {
    rust::Slice<const uint8_t> proof_slice{proof.data(), proof.size()};

    // Build the BoxedPathQueryVec from the chained queries.
    auto vec = grovedb_cxx::grovedb_path_query_vec_new();
    for (const auto* q : chained_queries) {
      grovedb_cxx::grovedb_path_query_vec_push(*vec, *q->m_impl->m_query);
    }

    auto result = grovedb_cxx::grovedb_verify_chained_queries(
        proof_slice, *first_query.m_impl->m_query, *vec
    );

    if (result.root_hash.size() != 32) {
      return Err(
          Error::Corruption(
              "root hash: expected 32 bytes, got " + std::to_string(result.root_hash.size())
          )
      );
    }
    Hash root_hash{};
    std::copy(result.root_hash.begin(), result.root_hash.end(), root_hash.begin());

    // Decode nested results: [u32 query_count][result_set₁][result_set₂]…
    wire::Reader r{{result.results.data(), result.results.size()}};
    auto qcount_r = r.U32();
    if (!qcount_r) {
      return Err(Error::Corruption("failed to decode chained query count"));
    }

    if (*qcount_r > wire::MAX_VECTOR_SIZE) {
      return Err(Error::Corruption("chained query count exceeds maximum"));
    }

    std::vector<std::vector<PathKeyElement>> all_results;
    all_results.reserve(*qcount_r);
    for (uint32_t i{0}; i < *qcount_r; ++i) {
      auto count_r = r.U32();
      if (!count_r) {
        return Err(Error::Corruption("failed to decode chained result set count"));
      }
      if (*count_r > wire::MAX_VECTOR_SIZE) {
        return Err(Error::Corruption("chained result set count exceeds maximum"));
      }

      std::vector<PathKeyElement> entries;
      entries.reserve(*count_r);
      for (uint32_t j{0}; j < *count_r; ++j) {
        PathKeyElement entry;
        auto path_r = wire::Read<Path>(r);
        if (!path_r) {
          return Err(Error::Corruption("failed to decode chained result path"));
        }
        entry.m_path = std::move(*path_r);

        auto key_r = r.Bytes();
        if (!key_r) {
          return Err(Error::Corruption("failed to decode chained result key"));
        }
        entry.m_key = std::move(*key_r);

        auto has_r = r.U8();
        if (!has_r) {
          return Err(Error::Corruption("failed to decode chained result element flag"));
        }
        if (*has_r) {
          auto elem_r = r.Bytes();
          if (!elem_r) {
            return Err(Error::Corruption("failed to decode chained result element"));
          }
          entry.m_element = Element::FromData(std::move(*elem_r));
        }
        entries.push_back(std::move(entry));
      }
      all_results.push_back(std::move(entries));
    }

    return Costed<CVR>{CVR{root_hash, std::move(all_results)}, convert_cost(result.cost)};
  } catch (const std::exception& e) {
    return Err(StringToError(e.what()));
  }
}
} // namespace grovedb

// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <unordered_lru_cache.h>

#include <cstdint>
#include <functional>
#include <vector>

namespace {

struct U64Hasher {
    size_t operator()(uint64_t k) const { return std::hash<uint64_t>{}(k); }
};

} // anonymous namespace

// Baseline: insert into a cache that never triggers eviction
static void LruCacheInsertNoEviction(benchmark::Bench& bench)
{
    unordered_lru_cache<uint64_t, uint64_t, U64Hasher> cache(10000, 20000);
    uint64_t i = 0;
    bench.run([&] {
        cache.insert(i, i * 10);
        // reset before hitting threshold
        if (++i >= 10000) {
            cache.clear();
            i = 0;
        }
    });
}
BENCHMARK(LruCacheInsertNoEviction, benchmark::PriorityLevel::HIGH);

// Insert with eviction: measures the amortized cost including batch eviction spikes
static void LruCacheInsertWithEviction(benchmark::Bench& bench)
{
    // maxSize=1000, truncateThreshold=2000
    // every 1001st insert past threshold triggers O(n log n) eviction
    unordered_lru_cache<uint64_t, uint64_t, U64Hasher> cache(1000, 2000);
    uint64_t i = 0;
    bench.run([&] {
        cache.insert(i, i * 10);
        ++i;
    });
}
BENCHMARK(LruCacheInsertWithEviction, benchmark::PriorityLevel::HIGH);

// Get on cache hit (small value type — int)
static void LruCacheGetHitSmall(benchmark::Bench& bench)
{
    constexpr size_t SIZE = 1000;
    unordered_lru_cache<uint64_t, uint64_t, U64Hasher> cache(SIZE, SIZE * 2);
    for (uint64_t i = 0; i < SIZE; i++) {
        cache.insert(i, i * 10);
    }
    uint64_t val{0};
    uint64_t i = 0;
    bench.run([&] {
        cache.get(i % SIZE, val);
        ++i;
    });
}
BENCHMARK(LruCacheGetHitSmall, benchmark::PriorityLevel::HIGH);

// Get on cache hit (large value — 1KB vector, simulates CQuorumSnapshot)
static void LruCacheGetHitLarge(benchmark::Bench& bench)
{
    constexpr size_t SIZE = 100;
    using LargeVal = std::vector<uint8_t>;
    unordered_lru_cache<uint64_t, LargeVal, U64Hasher> cache(SIZE, SIZE * 2);
    for (uint64_t i = 0; i < SIZE; i++) {
        cache.insert(i, LargeVal(1024, static_cast<uint8_t>(i)));
    }
    LargeVal val;
    uint64_t i = 0;
    bench.run([&] {
        cache.get(i % SIZE, val);
        ++i;
    });
}
BENCHMARK(LruCacheGetHitLarge, benchmark::PriorityLevel::HIGH);

// Get on cache miss
static void LruCacheGetMiss(benchmark::Bench& bench)
{
    constexpr size_t SIZE = 1000;
    unordered_lru_cache<uint64_t, uint64_t, U64Hasher> cache(SIZE, SIZE * 2);
    for (uint64_t i = 0; i < SIZE; i++) {
        cache.insert(i, i * 10);
    }
    uint64_t val{0};
    uint64_t i = SIZE; // start past all inserted keys
    bench.run([&] {
        cache.get(i++, val);
    });
}
BENCHMARK(LruCacheGetMiss, benchmark::PriorityLevel::HIGH);

// exists() check (hit)
static void LruCacheExistsHit(benchmark::Bench& bench)
{
    constexpr size_t SIZE = 1000;
    unordered_lru_cache<uint64_t, uint64_t, U64Hasher> cache(SIZE, SIZE * 2);
    for (uint64_t i = 0; i < SIZE; i++) {
        cache.insert(i, i);
    }
    uint64_t i = 0;
    bench.run([&] {
        cache.exists(i % SIZE);
        ++i;
    });
}
BENCHMARK(LruCacheExistsHit, benchmark::PriorityLevel::HIGH);

// Mixed workload: 80% get (hit), 20% insert (new keys) — simulates real cache usage
static void LruCacheMixedWorkload(benchmark::Bench& bench)
{
    constexpr size_t SIZE = 1000;
    unordered_lru_cache<uint64_t, uint64_t, U64Hasher> cache(SIZE, SIZE * 2);
    // pre-fill
    for (uint64_t i = 0; i < SIZE; i++) {
        cache.insert(i, i * 10);
    }
    uint64_t val{0};
    uint64_t i = 0;
    uint64_t next_new_key = SIZE;
    bench.run([&] {
        if (i % 5 == 0) {
            // 20% inserts of new keys
            uint64_t key = next_new_key++;
            cache.insert(key, key * 10);
        } else {
            // 80% gets of existing keys (may miss after eviction)
            cache.get(next_new_key - 1 - (i % SIZE), val);
        }
        ++i;
    });
}
BENCHMARK(LruCacheMixedWorkload, benchmark::PriorityLevel::HIGH);

// Erase performance
static void LruCacheErase(benchmark::Bench& bench)
{
    constexpr size_t SIZE = 1000;
    unordered_lru_cache<uint64_t, uint64_t, U64Hasher> cache(SIZE, SIZE * 2);
    uint64_t i = 0;
    bench.run([&] {
        // insert then erase to keep steady state
        cache.insert(i, i * 10);
        cache.erase(i);
        ++i;
    });
}
BENCHMARK(LruCacheErase, benchmark::PriorityLevel::HIGH);

// Bool cache — matches the common hasSigForIdCache / mapHasMinedCommitmentCache pattern
static void LruCacheBoolInsertGet(benchmark::Bench& bench)
{
    constexpr size_t SIZE = 30000; // matches hasSigForIdCache size
    unordered_lru_cache<uint64_t, bool, U64Hasher> cache(SIZE, SIZE * 2);
    bool val{false};
    uint64_t i = 0;
    bench.run([&] {
        if (i < SIZE) {
            cache.insert(i, (i % 2) == 0);
        } else {
            cache.get(i - SIZE, val);
        }
        if (++i >= SIZE * 2) {
            cache.clear();
            i = 0;
        }
    });
}
BENCHMARK(LruCacheBoolInsertGet, benchmark::PriorityLevel::HIGH);

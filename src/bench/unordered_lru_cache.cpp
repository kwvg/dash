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

} // namespace

static void LruCacheInsert(benchmark::Bench& bench)
{
    unordered_lru_cache<uint64_t, uint64_t, U64Hasher> cache(10000);
    uint64_t i{0};
    bench.run([&] {
        cache.insert(i, i * 10);
        if (++i >= 10000) {
            cache.clear();
            i = 0;
        }
    });
}
BENCHMARK(LruCacheInsert, benchmark::PriorityLevel::HIGH);

static void LruCacheInsertWithEviction(benchmark::Bench& bench)
{
    unordered_lru_cache<uint64_t, uint64_t, U64Hasher> cache(1000);
    uint64_t i{0};
    bench.run([&] {
        cache.insert(i, i * 10);
        ++i;
    });
}
BENCHMARK(LruCacheInsertWithEviction, benchmark::PriorityLevel::HIGH);

static void LruCacheGetHitSmall(benchmark::Bench& bench)
{
    constexpr size_t SIZE{1000};
    unordered_lru_cache<uint64_t, uint64_t, U64Hasher> cache(SIZE);
    for (uint64_t i{0}; i < SIZE; ++i) {
        cache.insert(i, i * 10);
    }
    uint64_t val{0};
    uint64_t i{0};
    bench.run([&] {
        cache.get(i % SIZE, val);
        ++i;
    });
}
BENCHMARK(LruCacheGetHitSmall, benchmark::PriorityLevel::HIGH);

static void LruCacheGetHitLarge(benchmark::Bench& bench)
{
    constexpr size_t SIZE{100};
    using LargeVal = std::vector<uint8_t>;
    unordered_lru_cache<uint64_t, LargeVal, U64Hasher> cache(SIZE);
    for (uint64_t i{0}; i < SIZE; ++i) {
        cache.insert(i, LargeVal(1024, static_cast<uint8_t>(i)));
    }
    LargeVal val;
    uint64_t i{0};
    bench.run([&] {
        cache.get(i % SIZE, val);
        ++i;
    });
}
BENCHMARK(LruCacheGetHitLarge, benchmark::PriorityLevel::HIGH);

static void LruCacheGetMiss(benchmark::Bench& bench)
{
    constexpr size_t SIZE{1000};
    unordered_lru_cache<uint64_t, uint64_t, U64Hasher> cache(SIZE);
    for (uint64_t i{0}; i < SIZE; ++i) {
        cache.insert(i, i * 10);
    }
    uint64_t val{0};
    uint64_t i{SIZE};
    bench.run([&] {
        cache.get(i, val);
        ++i;
    });
}
BENCHMARK(LruCacheGetMiss, benchmark::PriorityLevel::HIGH);

static void LruCacheExistsHit(benchmark::Bench& bench)
{
    constexpr size_t SIZE{1000};
    unordered_lru_cache<uint64_t, uint64_t, U64Hasher> cache(SIZE);
    for (uint64_t i{0}; i < SIZE; ++i) {
        cache.insert(i, i);
    }
    uint64_t i{0};
    bench.run([&] {
        cache.exists(i % SIZE);
        ++i;
    });
}
BENCHMARK(LruCacheExistsHit, benchmark::PriorityLevel::HIGH);

static void LruCacheMixedWorkload(benchmark::Bench& bench)
{
    constexpr size_t SIZE{1000};
    unordered_lru_cache<uint64_t, uint64_t, U64Hasher> cache(SIZE);
    for (uint64_t i{0}; i < SIZE; ++i) {
        cache.insert(i, i * 10);
    }
    uint64_t val{0};
    uint64_t i{0};
    uint64_t next_new_key{SIZE};
    bench.run([&] {
        if (i % 5 == 0) {
            uint64_t key{next_new_key++};
            cache.insert(key, key * 10);
        } else {
            cache.get(next_new_key - 1 - (i % SIZE), val);
        }
        ++i;
    });
}
BENCHMARK(LruCacheMixedWorkload, benchmark::PriorityLevel::HIGH);

static void LruCacheErase(benchmark::Bench& bench)
{
    constexpr size_t SIZE{1000};
    unordered_lru_cache<uint64_t, uint64_t, U64Hasher> cache(SIZE);
    uint64_t i{0};
    bench.run([&] {
        cache.insert(i, i * 10);
        cache.erase(i);
        ++i;
    });
}
BENCHMARK(LruCacheErase, benchmark::PriorityLevel::HIGH);

static void LruCacheBoolInsertGet(benchmark::Bench& bench)
{
    constexpr size_t SIZE{30000};
    unordered_lru_cache<uint64_t, bool, U64Hasher> cache(SIZE);
    bool val{false};
    uint64_t i{0};
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

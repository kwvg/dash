// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <unordered_lru_cache.h>

#include <test/util/setup_common.h>

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

namespace {

struct IntHasher {
    size_t operator()(int k) const { return std::hash<int>{}(k); }
};

struct LargeValue {
    std::vector<uint8_t> data;

    explicit LargeValue(size_t size) : data(size, 0x42) {}
};

using SmallCache = unordered_lru_cache<int, int, IntHasher, 5>;
using StringCache = unordered_lru_cache<int, std::string, IntHasher, 3>;
using LargeCache = unordered_lru_cache<int, LargeValue, IntHasher, 3>;

} // namespace

BOOST_FIXTURE_TEST_SUITE(unordered_lru_cache_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(insert_and_get)
{
    SmallCache cache;

    BOOST_CHECK(cache.get(1) == nullptr);

    cache.insert(1, 100);
    const auto* val = cache.get(1);
    BOOST_CHECK(val != nullptr);
    BOOST_CHECK_EQUAL(*val, 100);
}

BOOST_AUTO_TEST_CASE(insert_overwrites_existing)
{
    SmallCache cache;

    cache.insert(1, 100);
    cache.insert(1, 200);
    const auto* val = cache.get(1);
    BOOST_CHECK(val != nullptr);
    BOOST_CHECK_EQUAL(*val, 200);
}

BOOST_AUTO_TEST_CASE(emplace_overwrites_existing)
{
    SmallCache cache;

    cache.emplace(1, 100);
    cache.emplace(1, 200);
    const auto* val = cache.get(1);
    BOOST_CHECK(val != nullptr);
    BOOST_CHECK_EQUAL(*val, 200);
}

BOOST_AUTO_TEST_CASE(get_miss_returns_nullptr)
{
    SmallCache cache;
    BOOST_CHECK(cache.get(999) == nullptr);
}

BOOST_AUTO_TEST_CASE(exists_hit_and_miss)
{
    SmallCache cache;

    cache.insert(1, 100);
    BOOST_CHECK(cache.exists(1));
    BOOST_CHECK(!cache.exists(2));
}

BOOST_AUTO_TEST_CASE(erase_removes_entry)
{
    SmallCache cache;

    cache.insert(1, 100);
    cache.erase(1);
    BOOST_CHECK(cache.get(1) == nullptr);
    BOOST_CHECK(!cache.exists(1));
}

BOOST_AUTO_TEST_CASE(erase_nonexistent_is_noop)
{
    SmallCache cache;
    cache.insert(1, 100);
    cache.erase(999);
    const auto* val = cache.get(1);
    BOOST_CHECK(val != nullptr);
    BOOST_CHECK_EQUAL(*val, 100);
}

BOOST_AUTO_TEST_CASE(clear_empties_cache)
{
    SmallCache cache;
    for (int i{0}; i < 5; ++i) {
        cache.insert(i, i * 10);
    }
    cache.clear();
    for (int i{0}; i < 5; ++i) {
        BOOST_CHECK(!cache.exists(i));
    }
}

BOOST_AUTO_TEST_CASE(max_size_returns_configured_value)
{
    SmallCache cache;
    BOOST_CHECK_EQUAL(cache.max_size(), 5u);

    unordered_lru_cache<int, int, IntHasher> cache2(42);
    BOOST_CHECK_EQUAL(cache2.max_size(), 42u);
}

BOOST_AUTO_TEST_CASE(size_and_empty)
{
    SmallCache cache;
    BOOST_CHECK(cache.empty());
    BOOST_CHECK_EQUAL(cache.size(), 0u);

    cache.insert(1, 10);
    BOOST_CHECK(!cache.empty());
    BOOST_CHECK_EQUAL(cache.size(), 1u);

    cache.insert(2, 20);
    cache.insert(3, 30);
    BOOST_CHECK_EQUAL(cache.size(), 3u);

    cache.erase(2);
    BOOST_CHECK_EQUAL(cache.size(), 2u);

    cache.clear();
    BOOST_CHECK(cache.empty());
    BOOST_CHECK_EQUAL(cache.size(), 0u);
}

BOOST_AUTO_TEST_CASE(string_values)
{
    StringCache cache;

    cache.insert(1, "hello");
    cache.insert(2, "world");

    const auto* v1 = cache.get(1);
    BOOST_CHECK(v1 != nullptr);
    BOOST_CHECK_EQUAL(*v1, "hello");
    const auto* v2 = cache.get(2);
    BOOST_CHECK(v2 != nullptr);
    BOOST_CHECK_EQUAL(*v2, "world");
}

BOOST_AUTO_TEST_CASE(eviction_removes_least_recently_used)
{
    SmallCache cache;

    // insert 11 items, triggering eviction(s)
    for (int i{0}; i <= 10; ++i) {
        cache.insert(i, i * 10);
    }

    // after eviction, only the 5 most recently inserted should survive
    for (int i{0}; i <= 5; ++i) {
        BOOST_CHECK_MESSAGE(cache.get(i) == nullptr, "key " + std::to_string(i) + " should have been evicted");
    }
    for (int i{6}; i <= 10; ++i) {
        const auto* val = cache.get(i);
        BOOST_CHECK_MESSAGE(val != nullptr, "key " + std::to_string(i) + " should still exist");
        BOOST_CHECK_EQUAL(*val, i * 10);
    }
}

BOOST_AUTO_TEST_CASE(get_promotes_entry_in_lru_order)
{
    unordered_lru_cache<int, int, IntHasher> cache(5);

    for (int i{0}; i < 5; ++i) {
        cache.insert(i, i * 10);
    }

    // promote key 0 via get()
    cache.get(0);

    // insert enough items to trigger eviction
    for (int i{5}; i <= 7; ++i) {
        cache.insert(i, i * 10);
    }

    // key 0 should survive (promoted by get), unpromoted keys 1-3 should be evicted
    const auto* val = cache.get(0);
    BOOST_CHECK_MESSAGE(val != nullptr, "key 0 should survive (promoted by get)");
    BOOST_CHECK_EQUAL(*val, 0);
    BOOST_CHECK(!cache.exists(1));
    BOOST_CHECK(!cache.exists(2));
    BOOST_CHECK(!cache.exists(3));
}

BOOST_AUTO_TEST_CASE(exists_promotes_entry_in_lru_order)
{
    unordered_lru_cache<int, int, IntHasher> cache(5);

    for (int i{0}; i < 5; ++i) {
        cache.insert(i, i * 10);
    }

    cache.exists(0);

    // insert enough to trigger eviction
    for (int i{5}; i <= 7; ++i) {
        cache.insert(i, i * 10);
    }

    BOOST_CHECK(cache.exists(0));
    BOOST_CHECK(!cache.exists(1));
}

BOOST_AUTO_TEST_CASE(insert_overwrite_promotes_entry)
{
    unordered_lru_cache<int, int, IntHasher> cache(5);

    for (int i{0}; i < 5; ++i) {
        cache.insert(i, i * 10);
    }

    cache.insert(0, 999);

    // insert enough to trigger eviction
    for (int i{5}; i <= 7; ++i) {
        cache.insert(i, i * 10);
    }

    const auto* val = cache.get(0);
    BOOST_CHECK(val != nullptr);
    BOOST_CHECK_EQUAL(*val, 999);
    BOOST_CHECK(!cache.exists(1));
}

BOOST_AUTO_TEST_CASE(size_bounded_by_truncate_threshold)
{
    SmallCache cache;

    for (int i{0}; i < 100; ++i) {
        cache.insert(i, i * 10);
        BOOST_CHECK(cache.size() <= cache.truncate_threshold());
    }
    BOOST_CHECK(cache.size() >= cache.max_size());
    BOOST_CHECK(cache.size() <= cache.truncate_threshold());
}

BOOST_AUTO_TEST_CASE(large_values)
{
    LargeCache cache;

    cache.insert(1, LargeValue(1024));
    cache.insert(2, LargeValue(2048));
    cache.insert(3, LargeValue(4096));

    const auto* v1 = cache.get(1);
    BOOST_CHECK(v1 != nullptr);
    BOOST_CHECK_EQUAL(v1->data.size(), 1024u);
    const auto* v2 = cache.get(2);
    BOOST_CHECK(v2 != nullptr);
    BOOST_CHECK_EQUAL(v2->data.size(), 2048u);
    const auto* v3 = cache.get(3);
    BOOST_CHECK(v3 != nullptr);
    BOOST_CHECK_EQUAL(v3->data.size(), 4096u);
}

BOOST_AUTO_TEST_CASE(large_value_eviction)
{
    LargeCache cache;

    // insert 7 items to trigger eviction
    for (int i{0}; i < 7; ++i) {
        cache.insert(i, LargeValue(1024 * (i + 1)));
    }

    // the 3 most recently inserted (4, 5, 6) should survive
    int count{0};
    for (int i{0}; i < 7; ++i) {
        if (cache.get(i) != nullptr) ++count;
    }
    BOOST_CHECK_EQUAL(count, 3);

    const auto* v4 = cache.get(4);
    BOOST_CHECK(v4 != nullptr);
    BOOST_CHECK_EQUAL(v4->data.size(), 5 * 1024u);
    const auto* v5 = cache.get(5);
    BOOST_CHECK(v5 != nullptr);
    BOOST_CHECK_EQUAL(v5->data.size(), 6 * 1024u);
    const auto* v6 = cache.get(6);
    BOOST_CHECK(v6 != nullptr);
    BOOST_CHECK_EQUAL(v6->data.size(), 7 * 1024u);
}

BOOST_AUTO_TEST_CASE(single_element_cache)
{
    unordered_lru_cache<int, int, IntHasher> cache(1);

    cache.insert(1, 100);
    const auto* val = cache.get(1);
    BOOST_CHECK(val != nullptr);
    BOOST_CHECK_EQUAL(*val, 100);

    cache.insert(2, 200);
    cache.insert(3, 300);

    int count{0};
    for (int i{1}; i <= 3; ++i) {
        if (cache.exists(i)) ++count;
    }
    BOOST_CHECK_EQUAL(count, 1);
    val = cache.get(3);
    BOOST_CHECK(val != nullptr);
    BOOST_CHECK_EQUAL(*val, 300);
}

BOOST_AUTO_TEST_CASE(repeated_access_same_key)
{
    SmallCache cache;

    cache.insert(1, 100);

    for (int i{0}; i < 100; ++i) {
        const auto* val = cache.get(1);
        BOOST_CHECK(val != nullptr);
        BOOST_CHECK_EQUAL(*val, 100);
    }
}

BOOST_AUTO_TEST_CASE(interleaved_insert_get_erase)
{
    SmallCache cache;

    cache.insert(1, 10);
    cache.insert(2, 20);
    cache.get(1);
    cache.erase(2);
    cache.insert(3, 30);
    BOOST_CHECK(!cache.exists(2));
    const auto* v1 = cache.get(1);
    BOOST_CHECK(v1 != nullptr);
    BOOST_CHECK_EQUAL(*v1, 10);
    const auto* v3 = cache.get(3);
    BOOST_CHECK(v3 != nullptr);
    BOOST_CHECK_EQUAL(*v3, 30);
}

BOOST_AUTO_TEST_CASE(stress_many_insertions)
{
    unordered_lru_cache<int, int, IntHasher> cache(100);

    for (int i{0}; i < 1000; ++i) {
        cache.insert(i, i * 10);

        if (i > 0 && i % 10 == 0) {
            cache.get(i - 5);
        }
    }

    for (int i{999}; i >= 950; --i) {
        const auto* val = cache.get(i);
        BOOST_CHECK_MESSAGE(val != nullptr, "key " + std::to_string(i) + " should exist");
        BOOST_CHECK_EQUAL(*val, i * 10);
    }
}

BOOST_AUTO_TEST_CASE(pair_key_type)
{
    struct PairHasher {
        size_t operator()(const std::pair<int, int>& p) const {
            return std::hash<int>{}(p.first) ^ (std::hash<int>{}(p.second) << 16);
        }
    };

    unordered_lru_cache<std::pair<int, int>, bool, PairHasher> cache(5);

    cache.insert({1, 2}, true);
    cache.insert({3, 4}, false);

    const auto* v1 = cache.get({1, 2});
    BOOST_CHECK(v1 != nullptr);
    BOOST_CHECK_EQUAL(*v1, true);
    const auto* v2 = cache.get({3, 4});
    BOOST_CHECK(v2 != nullptr);
    BOOST_CHECK_EQUAL(*v2, false);
    BOOST_CHECK(cache.get({5, 6}) == nullptr);
}

BOOST_AUTO_TEST_CASE(eviction_order_with_mixed_access_patterns)
{
    unordered_lru_cache<int, int, IntHasher> cache(5);

    for (int i{0}; i < 5; ++i) {
        cache.insert(i, i * 10);
    }

    // promote keys 0, 2, 4 via different methods
    cache.get(0);
    cache.exists(2);
    cache.insert(4, 40);

    // insert enough to trigger multiple evictions, clearing all old entries
    for (int i{5}; i <= 12; ++i) {
        cache.insert(i, i * 10);
    }

    // only the 5 most recently inserted should survive
    for (int i{8}; i <= 12; ++i) {
        BOOST_CHECK_MESSAGE(cache.exists(i), "key " + std::to_string(i) + " should survive");
    }
    // unpromoted keys should definitely be evicted
    BOOST_CHECK(!cache.exists(1));
    BOOST_CHECK(!cache.exists(3));
}

BOOST_AUTO_TEST_CASE(get_returns_correct_value_after_overwrite)
{
    SmallCache cache;

    cache.insert(1, 100);
    cache.insert(1, 200);
    cache.insert(1, 300);

    const auto* val = cache.get(1);
    BOOST_CHECK(val != nullptr);
    BOOST_CHECK_EQUAL(*val, 300);
}

BOOST_AUTO_TEST_CASE(erase_then_reinsert)
{
    SmallCache cache;

    cache.insert(1, 100);
    cache.erase(1);
    cache.insert(1, 200);

    const auto* val = cache.get(1);
    BOOST_CHECK(val != nullptr);
    BOOST_CHECK_EQUAL(*val, 200);
}

BOOST_AUTO_TEST_CASE(clear_then_reuse)
{
    SmallCache cache;

    for (int i{0}; i < 5; ++i) {
        cache.insert(i, i);
    }
    cache.clear();
    BOOST_CHECK_EQUAL(cache.max_size(), 5u);

    cache.insert(10, 100);
    const auto* val = cache.get(10);
    BOOST_CHECK(val != nullptr);
    BOOST_CHECK_EQUAL(*val, 100);
}

BOOST_AUTO_TEST_SUITE_END()

// Copyright (c) 2019-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UNORDERED_LRU_CACHE_H
#define BITCOIN_UNORDERED_LRU_CACHE_H

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

template <typename Key, typename Value, typename Hasher, size_t MaxSize = 0>
class unordered_lru_cache
{
private:
    using MapType = std::unordered_map<Key, std::pair<Value, int64_t>, Hasher>;

    MapType m_map;
    size_t m_max_size;
    size_t m_truncate_threshold;
    int64_t m_access_counter{0};

public:
    explicit unordered_lru_cache(size_t max_size = MaxSize) :
        m_max_size(max_size),
        m_truncate_threshold(max_size + std::max<size_t>(max_size / 2, 1))
    {
        assert(max_size != 0);
    }

    size_t max_size() const { return m_max_size; }

    template <typename Value2>
    void _emplace(const Key& key, Value2&& v)
    {
        auto it = m_map.find(key);
        if (it == m_map.end()) {
            m_map.emplace(key, std::make_pair(std::forward<Value2>(v), m_access_counter++));
        } else {
            it->second.first = std::forward<Value2>(v);
            it->second.second = m_access_counter++;
        }
        truncate_if_needed();
    }

    void emplace(const Key& key, Value&& v)
    {
        _emplace(key, std::move(v));
    }

    void insert(const Key& key, const Value& v)
    {
        _emplace(key, v);
    }

    const Value* get(const Key& key)
    {
        auto it = m_map.find(key);
        if (it != m_map.end()) {
            it->second.second = m_access_counter++;
            return &it->second.first;
        }
        return nullptr;
    }

    bool exists(const Key& key)
    {
        auto it = m_map.find(key);
        if (it != m_map.end()) {
            it->second.second = m_access_counter++;
            return true;
        }
        return false;
    }

    void erase(const Key& key)
    {
        m_map.erase(key);
    }

    void clear()
    {
        m_map.clear();
    }

private:
    void truncate_if_needed()
    {
        if (m_map.size() <= m_truncate_threshold) {
            return;
        }

        using Iterator = typename MapType::iterator;

        std::vector<Iterator> vec;
        vec.reserve(m_map.size());
        for (auto it = m_map.begin(); it != m_map.end(); ++it) {
            vec.emplace_back(it);
        }
        // partition: keep the m_max_size most recently accessed entries
        std::nth_element(vec.begin(), vec.begin() + m_max_size, vec.end(),
            [](const Iterator& a, const Iterator& b) {
                return a->second.second > b->second.second;
            });

        for (size_t i = m_max_size; i < vec.size(); ++i) {
            m_map.erase(vec[i]);
        }
    }
};

#endif // BITCOIN_UNORDERED_LRU_CACHE_H

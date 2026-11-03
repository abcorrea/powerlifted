#ifndef SEARCH_OPEN_LISTS_KEYED_OPEN_LIST_H
#define SEARCH_OPEN_LISTS_KEYED_OPEN_LIST_H

#include <array>
#include <cstddef>
#include <functional>
#include <queue>
#include <unordered_map>
#include <vector>

/*
  Open list of state ids with lexicographically ordered integer keys of
  length K, and FIFO order among entries with the same key. The entries are
  kept in one bucket per key, and the keys of the non-empty buckets in a
  min-heap.
*/
template <int K>
class KeyedOpenList {
    using Key = std::array<int, K>;
    struct KeyHash {
        size_t operator()(const Key &key) const
        {
            size_t h = 0;
            for (int value : key)
                h = h * 0x9E3779B97F4A7C15ULL + std::hash<int>()(value);
            return h;
        }
    };
    struct Bucket {
        std::vector<int> ids;
        size_t next = 0;
    };
    std::unordered_map<Key, Bucket, KeyHash> buckets;
    std::priority_queue<Key, std::vector<Key>, std::greater<Key>> keys;
    size_t num_entries = 0;

public:
    void push(const Key &key, int id)
    {
        auto [it, inserted] = buckets.try_emplace(key);
        if (inserted)
            keys.push(key);
        it->second.ids.push_back(id);
        ++num_entries;
    }

    int pop()
    {
        auto it = buckets.find(keys.top());
        Bucket &bucket = it->second;
        int id = bucket.ids[bucket.next++];
        if (bucket.next == bucket.ids.size()) {
            buckets.erase(it);
            keys.pop();
        }
        --num_entries;
        return id;
    }

    bool empty() const { return num_entries == 0; }
};

#endif  // SEARCH_OPEN_LISTS_KEYED_OPEN_LIST_H

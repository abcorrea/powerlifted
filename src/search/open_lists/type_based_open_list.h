#ifndef SEARCH_OPEN_LISTS_TYPE_BASED_OPEN_LIST_H
#define SEARCH_OPEN_LISTS_TYPE_BASED_OPEN_LIST_H

#include <cstddef>
#include <random>
#include <unordered_map>
#include <utility>
#include <vector>

/*
  Type-based exploration (Xie et al., AAAI 2014): state ids are grouped into
  buckets by their key (a pair of integers, e.g. heuristic value and g), and
  pop() removes a random entry of a random bucket, so that all keys are
  explored at the same rate.
*/
class TypeBasedOpenList {
    struct KeyHash {
        size_t operator()(const std::pair<int, int> &key) const
        {
            return std::hash<long long>()((static_cast<long long>(key.first) << 32) ^
                                          static_cast<unsigned>(key.second));
        }
    };
    std::vector<std::pair<std::pair<int, int>, std::vector<int>>> buckets;
    std::unordered_map<std::pair<int, int>, size_t, KeyHash> bucket_index;
    std::mt19937 rng;

    size_t random_index(size_t size) { return rng() % size; }

public:
    explicit TypeBasedOpenList(unsigned seed = 1) : rng(seed) {}

    void push(const std::pair<int, int> &key, int id)
    {
        auto it = bucket_index.find(key);
        if (it == bucket_index.end()) {
            bucket_index.emplace(key, buckets.size());
            buckets.emplace_back(key, std::vector<int>{id});
        }
        else {
            buckets[it->second].second.push_back(id);
        }
    }

    int pop()
    {
        size_t b = random_index(buckets.size());
        std::vector<int> &bucket = buckets[b].second;
        size_t pos = random_index(bucket.size());
        int id = bucket[pos];
        bucket[pos] = bucket.back();
        bucket.pop_back();
        if (bucket.empty()) {
            bucket_index.erase(buckets[b].first);
            if (b + 1 != buckets.size()) {
                buckets[b] = std::move(buckets.back());
                bucket_index[buckets[b].first] = b;
            }
            buckets.pop_back();
        }
        return id;
    }

    bool empty() const { return buckets.empty(); }
};

#endif  // SEARCH_OPEN_LISTS_TYPE_BASED_OPEN_LIST_H

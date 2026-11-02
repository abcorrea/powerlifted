#ifndef SEARCH_OPEN_LISTS_KEYED_OPEN_LIST_H
#define SEARCH_OPEN_LISTS_KEYED_OPEN_LIST_H

#include <array>
#include <cstddef>
#include <deque>
#include <map>

/*
  Open list of state ids with lexicographically ordered integer keys of
  length K, and FIFO order among entries with the same key.
*/
template <int K>
class KeyedOpenList {
    std::map<std::array<int, K>, std::deque<int>> buckets;
    size_t num_entries = 0;

public:
    void push(const std::array<int, K> &key, int id)
    {
        buckets[key].push_back(id);
        ++num_entries;
    }

    int pop()
    {
        auto it = buckets.begin();
        int id = it->second.front();
        it->second.pop_front();
        if (it->second.empty())
            buckets.erase(it);
        --num_entries;
        return id;
    }

    bool empty() const { return num_entries == 0; }
};

#endif  // SEARCH_OPEN_LISTS_KEYED_OPEN_LIST_H

#ifndef SEARCH_OPEN_LISTS_TIEBREAKING_OPEN_LIST_H_
#define SEARCH_OPEN_LISTS_TIEBREAKING_OPEN_LIST_H_

#include <deque>
#include <map>

#include "../search_engines/nodes.h"

struct CompareTieBreakingEntries {
    bool operator()(const std::vector<int> &lhs, const std::vector<int> &rhs) const {
        assert(lhs.size()==rhs.size());
        for (size_t i = 0; i < lhs.size(); ++i) {
            if (lhs[i]!=rhs[i]) {
                return lhs[i] < rhs[i];
            }
        }
        return false;
    }
};

class TieBreakingOpenList {
    typedef std::deque<StateID> Bucket;

    std::map<std::vector<int>, Bucket, CompareTieBreakingEntries> buckets;
    int size;

public:
    TieBreakingOpenList() : size(0) {}

    void do_insertion(const StateID &entry, const std::vector<int>& key) {
        buckets[key].push_back(entry);
        ++size;
    }

    StateID remove_min() {
        assert(size > 0);
        auto it = buckets.begin();
        assert(it != buckets.end());
        Bucket &bucket = it->second;
        assert(!bucket.empty());
        StateID result = bucket.front();
        bucket.pop_front();
        if (bucket.empty())
            buckets.erase(it);
        --size;
        return result;
    }

    bool empty() {
        return size == 0;
    }

};


#endif //SEARCH_OPEN_LISTS_TIEBREAKING_OPEN_LIST_H_

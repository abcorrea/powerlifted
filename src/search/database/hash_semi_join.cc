
#include "hash_semi_join.h"
#include "../hash_structures.h"
#include "table.h"
#include "utils.h"

#include <unordered_set>
#include <vector>

using namespace std;

size_t hash_semi_join(Table &t1, const Table &t2) {
    auto matches = compute_matching_columns(t1, t2);

    if (matches.empty()) {
        return t1.tuples.size();
    }

    // Build phase: collect the set of join keys present in t2
    unordered_set<vector<int>, TupleHash> t2_keys;
    {
        vector<int> key(matches.size());
        for (const auto &tuple : t2.tuples) {
            for (size_t i = 0; i < matches.size(); ++i) {
                key[i] = tuple[matches[i].second];
            }
            t2_keys.insert(key);
        }
    }

    // Probe phase: keep t1 tuples whose join key exists in t2
    vector<Table::tuple_t> new_tuples;
    {
        vector<int> key(matches.size());
        for (const auto &tuple : t1.tuples) {
            for (size_t i = 0; i < matches.size(); ++i) {
                key[i] = tuple[matches[i].first];
            }
            if (t2_keys.count(key) > 0) {
                new_tuples.push_back(tuple);
            }
        }
    }

    t1.tuples = std::move(new_tuples);
    return t1.tuples.size();
}

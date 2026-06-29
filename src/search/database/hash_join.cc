
#include "hash_join.h"
#include "../hash_structures.h"
#include "table.h"
#include "utils.h"

#include <cassert>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;

void hash_join(Table &t1, const Table &t2) {
    std::vector<int> matches1, matches2;
    compute_matching_columns(t1, t2, matches1, matches2);
    assert(matches1.size() == matches2.size());

    if (matches1.empty()) {
        cartesian_product(t1, t2);
        return;
    }

    // Build phase: index t1 tuples by their join key
    unordered_map<vector<int>, vector<Table::tuple_t>, TupleHash> hash_join_map;
    {
        vector<int> key(matches1.size());
        for (const auto &tuple : t1.tuples) {
            for (size_t i = 0; i < matches1.size(); ++i) {
                key[i] = tuple[matches1[i]];
            }
            hash_join_map[key].push_back(tuple);
        }
    }

    // Precompute which columns of t2 to keep (not in the join key)
    vector<bool> keep_col(t2.tuple_index.size(), true);
    for (int m : matches2) {
        keep_col[m] = false;
    }

    vector<int> t2_keep_indices;
    for (size_t j = 0; j < keep_col.size(); ++j) {
        if (keep_col[j]) {
            t2_keep_indices.push_back(j);
            t1.tuple_index.push_back(t2.tuple_index[j]);
        }
    }

    // Probe phase: scan t2 and look up matching t1 tuples
    vector<Table::tuple_t> new_tuples;
    {
        vector<int> key(matches2.size());
        for (const auto &tuple : t2.tuples) {
            for (size_t i = 0; i < matches2.size(); ++i) {
                key[i] = tuple[matches2[i]];
            }

            auto it = hash_join_map.find(key);
            if (it != hash_join_map.end()) {
                for (const auto &t1_tuple : it->second) {
                    Table::tuple_t combined;
                    combined.reserve(t1_tuple.size() + t2_keep_indices.size());
                    combined.insert(combined.end(), t1_tuple.begin(), t1_tuple.end());
                    for (int idx : t2_keep_indices) {
                        combined.push_back(tuple[idx]);
                    }
                    new_tuples.push_back(std::move(combined));
                }
            }
        }
    }
    t1.tuples = std::move(new_tuples);
}

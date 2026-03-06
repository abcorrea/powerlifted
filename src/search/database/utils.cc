
#include "utils.h"
#include "table.h"

using namespace std;

std::vector<std::pair<int, int>> compute_matching_columns(const Table &t1, const Table &t2) {
    vector<pair<int, int>> matches;
    auto sz1 = t1.tuple_index.size();
    auto sz2 = t2.tuple_index.size();
    for (size_t i = 0; i < sz1; ++i) {
        for (size_t j = 0; j < sz2; ++j) {
            if (t1.tuple_index[i] == t2.tuple_index[j]) {
                matches.emplace_back(i, j);
            }
        }
    }
    return matches;
}

void compute_matching_columns(const Table &t1, const Table &t2, std::vector<int>& matches1, std::vector<int>& matches2) {
    auto sz1 = t1.tuple_index.size();
    auto sz2 = t2.tuple_index.size();
    for (size_t i = 0; i < sz1; ++i) {
        for (size_t j = 0; j < sz2; ++j) {
            if (t1.tuple_index[i] == t2.tuple_index[j]) {
                matches1.push_back(i);
                matches2.push_back(j);
            }
        }
    }
}

void cartesian_product(Table &t1, const Table &t2) {
    t1.tuple_index.insert(t1.tuple_index.end(),
                          t2.tuple_index.begin(), t2.tuple_index.end());

    const size_t sz1 = t1.tuples.empty() ? 0 : t1.tuples[0].size();
    const size_t sz2 = t2.tuples.empty() ? 0 : t2.tuples[0].size();
    const size_t combined_sz = sz1 + sz2;

    vector<vector<int>> new_tuples;
    new_tuples.reserve(t1.tuples.size() * t2.tuples.size());

    for (const auto &tuple_t1 : t1.tuples) {
        for (const auto &tuple_t2 : t2.tuples) {
            vector<int> combined;
            combined.reserve(combined_sz);
            combined.insert(combined.end(), tuple_t1.begin(), tuple_t1.end());
            combined.insert(combined.end(), tuple_t2.begin(), tuple_t2.end());
            new_tuples.push_back(std::move(combined));
        }
    }
    t1.tuples = std::move(new_tuples);
}


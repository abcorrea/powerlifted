
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
    vector<pair<int, int>> matches;
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


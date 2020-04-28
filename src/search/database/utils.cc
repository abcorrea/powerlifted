
#include "utils.h"
#include "table.h"

#include <cassert>

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


std::vector<std::pair<int, int>> compute_matching_columns(const CompactTable &t1, const CompactTable &t2) {
    return compute_matching_columns(t1.table(), t2.table());
}

void CompactTable::invalidate(std::size_t i) {
    assert(i < valid.size() && valid[i]);
    valid[i] = false;
}
Table CompactTable::consolidate() const {
    std::vector<Table::tuple_t> tuples;
    tuples.reserve(num_valid());
    auto sz1 = tab.tuples.size();
    for (unsigned i = 0; i < sz1; ++i) {
        if (is_valid(i)) {
         tuples.emplace_back(tab.tuples[i]);
        }
    }

    return Table(std::move(tuples), Table::index_t(tab.tuple_index));
}


std::vector<CompactTable> convert_to_compact_tables(std::vector<Table>&& tables) {
    std::vector<CompactTable> cts;
    cts.reserve(tables.size());
    for (auto&& t:tables) {
        cts.emplace_back(std::move(t));
    }
    return cts;
}
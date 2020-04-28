
#ifndef DATABASE_HASH_JOIN_H
#define DATABASE_HASH_JOIN_H

#include "table.h"
#include <utility>
#include <vector>

class CompactTable {
protected:
    Table tab;
    std::vector<bool> valid;
    std::size_t nvalid;

public:
    explicit CompactTable(Table&& tab_) :
        tab(std::move(tab_)),
        valid(tab.tuples.size(), true),
        nvalid(tab.tuples.size())
    {}

    void invalidate(std::size_t i);

    inline bool is_valid(std::size_t i) const { return (bool) valid[i]; }

    std::size_t size() const { return tab.tuples.size(); }

    std::size_t num_valid() const { return nvalid; }

    const Table& table() const { return tab; }
    Table& table() { return tab; }
    const std::vector<Table::tuple_t>& tuples() const { return tab.tuples; }

    Table consolidate() const;
};

std::vector<CompactTable> convert_to_compact_tables(std::vector<Table>&& tables);

std::vector<std::pair<int, int>> compute_matching_columns(const Table &t1, const Table &t2);

std::vector<std::pair<int, int>> compute_matching_columns(const CompactTable &t1, const CompactTable &t2);

#endif //SEARCH_SEMI_JOIN_H

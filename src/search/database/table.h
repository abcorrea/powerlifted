#ifndef SEARCH_TABLE_H
#define SEARCH_TABLE_H

#include <utility>
#include <unordered_set>
#include <vector>

#include "../hash_structures.h"

class Table {
public:
    std::unordered_set<std::vector<int>, TupleHash> tuples;
    std::vector<int> tuple_index;

    Table(std::unordered_set<std::vector<int>, TupleHash> tuples,
          std::vector<int> tuple_index) : tuples(std::move(tuples)),
                                          tuple_index (std::move(tuple_index)) {}

    Table() = default;


};

struct OrderTable {
    bool operator()(const Table &t1, const Table &t2) const {
        return t1.tuple_index.size() < t2.tuple_index.size();
    }
};

struct InverseOrderTable {
    bool operator()(const Table &t1, const Table &t2) const {
        return t1.tuple_index.size() > t2.tuple_index.size();
    }
};


#endif //SEARCH_TABLE_H

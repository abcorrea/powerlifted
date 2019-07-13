#ifndef SEARCH_TABLE_H
#define SEARCH_TABLE_H

#include <utility>
#include <vector>

class Table {
public:
    std::vector<std::vector<int>> tuples;
    std::vector<int> tuple_index;

    Table(std::vector<std::vector<int>> tuples,
          std::vector<int> tuple_index) : tuples(std::move(tuples)),
                                          tuple_index (std::move(tuple_index)) {}

    Table() = default;


};

struct OrderTable {
    bool operator()(const Table &t1, const Table &t2) const {
        return t1.tuple_index.size() < t2.tuple_index.size();
    }
};


#endif //SEARCH_TABLE_H

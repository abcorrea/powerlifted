#include <utility>

#include <utility>

#ifndef SEARCH_TABLE_H
#define SEARCH_TABLE_H

#include <utility>
#include <vector>

class Table {
public:
    std::vector<std::vector<int>> tuples;
    std::vector<int> tuple_index;

    Table(std::vector<std::vector<int>> tuples, std::vector<int> tuple_index) : tuples(std::move(tuples)),
                                                                                tuple_index (std::move(tuple_index)) {}

    Table() = default;


};


#endif //SEARCH_TABLE_H

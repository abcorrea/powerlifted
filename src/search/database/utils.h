
#ifndef DATABASE_HASH_JOIN_H
#define DATABASE_HASH_JOIN_H

#include <vector>
#include <utility>

class Table;

std::vector<std::pair<int, int>> compute_matching_columns(const Table &t1, const Table &t2);


#endif //SEARCH_SEMI_JOIN_H

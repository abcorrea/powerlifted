
#ifndef DATABASE_HASH_JOIN_H
#define DATABASE_HASH_JOIN_H

#include <vector>
#include <utility>

class Table;

std::vector<std::pair<int, int>> compute_matching_columns(const Table &t1, const Table &t2);

void compute_matching_columns(const Table &t1, const Table &t2, std::vector<int>& matches1, std::vector<int>& matches2);

#endif //SEARCH_SEMI_JOIN_H

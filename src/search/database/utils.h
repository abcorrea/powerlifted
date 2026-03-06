
#ifndef DATABASE_UTILS_H
#define DATABASE_UTILS_H

#include <vector>
#include <utility>

class Table;

std::vector<std::pair<int, int>> compute_matching_columns(const Table &t1, const Table &t2);

void compute_matching_columns(const Table &t1, const Table &t2, std::vector<int>& matches1, std::vector<int>& matches2);

/**
 * @brief Compute the cartesian product of two tables, writing the result into t1.
 */
void cartesian_product(Table &t1, const Table &t2);

#endif //DATABASE_UTILS_H

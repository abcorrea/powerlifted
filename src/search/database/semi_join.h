#ifndef SEARCH_SEMI_JOIN_H
#define SEARCH_SEMI_JOIN_H

#include "table.h"

#include "../hash_structures.h"

#include <utility>
#include <vector>

/**
 * @brief Semi join two tables using loop-based approach. Result is written in
 * the table passed as first parameter.
 *
 * @details Obtain all indices that will be joined by looping over the indices. If no attribute
 * can be joined, return. Otherwise, loop over t1 and t2 comparing the matching attributes for
 * each tuple and join the ones matching.
 *
 * @param t1: Working table.  Table on the left of the semi-join.
 * @param t2: Table on the right of the semi-join.
 * @return  Size of the working table.
 */
int semi_join(Table &t1, Table &t2);

#endif //SEARCH_SEMI_JOIN_H

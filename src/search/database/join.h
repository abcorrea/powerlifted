#ifndef SEARCH_JOIN_H
#define SEARCH_JOIN_H

#include <utility>
#include <vector>

#include "table.h"

#include "../hash_structures.h"

/**
 * @brief Join two tables using loop-based approach.  Result is written in the table
 * passed as first parameter.
 *
 * @details First, loop over the indices and check which attributes match. If no attribute
 * can be joined, perform the Cartesian product. Otherwise, loop over both relations check
 * for each tuple whether they match or not.
 *
 * @param t1: Working table.  First table to be joined.
 * @param t2: Second table to be joined.
 */
void join(Table &t1, Table &t2);


#endif //SEARCH_JOIN_H

#ifndef SEARCH_HASH_SEMI_JOIN_H
#define SEARCH_HASH_SEMI_JOIN_H

#include "table.h"

#include "../hash_structures.h"

#include <utility>
#include <vector>

/**
 * @brief Semi-join two tables but using hash-based approach.
 *
 * @see semi_join.h
 * @see semi_join.cc
 * @see hash_join.h
 */
size_t hash_semi_join(Table &t1, Table &t2);


#endif //SEARCH_HASH_SEMI_JOIN_H

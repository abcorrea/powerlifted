#ifndef SEARCH_HASH_SEMI_JOIN_H
#define SEARCH_HASH_SEMI_JOIN_H

#include <cstddef>

class Table;

/**
 * @brief Semi-join two tables but using hash-based approach.
 *
 * @see semi_join.h
 * @see semi_join.cc
 * @see hash_join.h
 */
std::size_t hash_semi_join(Table &t1, const Table &t2);


#endif //SEARCH_HASH_SEMI_JOIN_H

#ifndef SEARCH_HASH_JOIN_H
#define SEARCH_HASH_JOIN_H

class Table;

/**
 * @brief Join two tables but using hash-based approach.
 *
 * @details First, prepare a hash map for t1. Each entry is a pair (K, T) where K is a key and T
 * is a tuple. The key K is the values of T for the attributes joining t1 to t2. Then, scan t2
 * and compute the key K' for each tuple T'. Join a tuple T' with all tuples in the hash map
 * with key K'.
 *
 * @see join.h
 * @see join.cc
 */
void hash_join(Table &t1, const Table &t2);

#endif //SEARCH_HASH_JOIN_H

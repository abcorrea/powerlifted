#ifndef SEARCH_UTILS_H
#define SEARCH_UTILS_H

#include <cassert>
#include <cstdint>
#include <unordered_map>
#include <vector>

/**
 * @brief Utility functions, mostly related to memory. Originally from Fast Downward.
 * @return
 */

int get_peak_memory_in_kb();

template<typename T>
int estimate_vector_bytes(int num_elements) {
  /*
        This estimate is based on a study of the C++ standard library
        that shipped with gcc around the year 2017. It does not claim to
        be accurate and may certainly be inaccurate for other compilers
        or compiler versions.
      */
  int size = 0;
  size += 2 * sizeof(void *);       // overhead for dynamic memory management
  size += sizeof(std::vector<T>);   // size of empty vector
  size += num_elements * sizeof(T); // size of actual entries
  return size;
}

template<typename T>
int _estimate_hash_table_bytes(int num_entries) {
  /*
        The same comments as for estimate_vector_bytes apply.
        Additionally, there may be alignment issues, especially on
        64-bit systems, that make this estimate too optimistic for
        certain cases.
      */

  assert(num_entries < (1 << 28));
  /*
        Having num_entries < 2^28 is necessary but not sufficient for
        the result value to not overflow. If we ever change this
        function to support larger data structures (using a size_t
        return value), we must update the list of bounds below (taken
        from the gcc library source).
      */
  int num_buckets = 0;
  const auto bounds = {
      2, 5, 11, 23, 47, 97, 199, 409, 823, 1741, 3469, 6949, 14033,
      28411, 57557, 116731, 236897, 480881, 976369, 1982627, 4026031,
      8175383, 16601593, 33712729, 68460391, 139022417, 282312799
  };

  for (int bound : bounds) {
    if (num_entries < bound) {
      num_buckets = bound;
      break;
    }
  }

  int size = 0;
  size += 2 * sizeof(void *);                            // overhead for dynamic memory management
  size += sizeof(T);                                     // empty container
  using Entry = typename T::value_type;
  size += num_entries * sizeof(Entry);                   // actual entries
  size += num_entries * sizeof(Entry *);                 // pointer to values
  size += num_entries * sizeof(void *);                  // pointer to next node
  size += num_buckets * sizeof(void *);                  // pointer to next bucket
  return size;
}

template<typename Key, typename Value, typename Hash>
int estimate_unordered_map_bytes(int num_entries) {
  // See comments for _estimate_hash_table_bytes.
  return _estimate_hash_table_bytes<std::unordered_map<Key, Value, Hash>>(num_entries);
}

/* Test if the product of two numbers is bounded by a third number.
   Safe against overflow. The caller must guarantee
   0 <= factor1, factor2 <= limit; failing this is an error. */
extern bool is_product_within_limit(long factor1, long factor2, long limit);

/* Test if the product of two numbers falls between the given inclusive lower
   and upper bounds. Safe against overflow. The caller must guarantee
   lower_limit < 0 and upper_limit >= 0; failing this is an error. */
extern bool is_product_within_limits(
    int factor1, int factor2, int lower_limit, int upper_limit);

#endif //SEARCH_UTILS_H

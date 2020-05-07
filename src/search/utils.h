#ifndef SEARCH_UTILS_H
#define SEARCH_UTILS_H

/* Test if the product of two numbers is bounded by a third number.
   Safe against overflow. The caller must guarantee
   0 <= factor1, factor2 <= limit; failing this is an error. */
extern bool is_product_within_limit(long factor1, long factor2, long limit);

/* Test if the product of two numbers falls between the given inclusive lower
   and upper bounds. Safe against overflow. The caller must guarantee
   lower_limit < 0 and upper_limit >= 0; failing this is an error. */
extern bool is_product_within_limits(int factor1, int factor2, int lower_limit,
                                     int upper_limit);

#endif // SEARCH_UTILS_H

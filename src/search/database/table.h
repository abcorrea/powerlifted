#ifndef SEARCH_TABLE_H
#define SEARCH_TABLE_H

#include <vector>

/**
 * @brief Data-structure containing a set of tuples and the indices corresponding to
 * the free variable index of each tuple position.
 */
class Table {
public:
    using tuple_t = std::vector<int>;

    /// @var tuples: the relation corresponding to the table, encoded as a vector of tuples
    std::vector<tuple_t> tuples;
    /// @var tuple_index: Indices of each variable in order
    std::vector<int> tuple_index;

    Table(std::vector<tuple_t> &&tuples, std::vector<int> &&tuple_index) :
        tuples(std::move(tuples)),
        tuple_index(std::move(tuple_index))
    {}

    bool index_is_variable(size_t i) const {
        return tuple_index[i] >= 0;
    }

    Table() = default;

    static const Table& EMPTY_TABLE();
};

/// @brief Order tables from lowest to highest arity.
struct OrderTable {
    bool operator()(const Table &t1, const Table &t2) const {
        return t1.tuple_index.size() <= t2.tuple_index.size();
    }
};

/// @brief Order tables from highest to lowerst arity.
struct InverseOrderTable {
    bool operator()(const Table &t1, const Table &t2) const {
        return t1.tuple_index.size() > t2.tuple_index.size();
    }
};

/// @brief Order tables from lowest to highest cardinality.
struct OrderByTableSize {
    bool operator()(const Table &t1, const Table &t2) const {
        return t1.tuples.size() < t2.tuples.size();
    }
};


#endif //SEARCH_TABLE_H

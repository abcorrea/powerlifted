#ifndef SEARCH_ORDERED_JOIN_SUCCESSOR_H
#define SEARCH_ORDERED_JOIN_SUCCESSOR_H

#include "generic_join_successor.h"

#include <vector>
#include <map>

/**
 * This class implements the successor generator using a full join ordered by the arity of the predicates.
 * The order is from the predicate with lowest arity to the one with largest.
 *
 */
template <typename OrderT>
class OrderedJoinSuccessorGenerator : public GenericJoinSuccessor {
    std::vector<std::vector<int>> precondition_to_order;

public:
    explicit OrderedJoinSuccessorGenerator(const Task &task);

    // @see generic_join_successor.h
    bool parse_precond_into_join_program(const PrecompiledActionData &adata,
                                         const DBState &state,
                                         std::vector<Table>& tables) override;

    Table instantiate(const ActionSchema &action, const DBState &state) override;

};

struct OrderTable {
    bool operator()(const std::pair<int, int> &t1, const std::pair<int, int> &t2) const {
        return t1.first < t2.first;
    }
};

struct InverseOrderTable {
    bool operator()(const std::pair<int, int> &t1, const std::pair<int, int> &t2) const {
        return t1.first > t2.first;
    }
};

#endif //SEARCH_ORDERED_JOIN_SUCCESSOR_H

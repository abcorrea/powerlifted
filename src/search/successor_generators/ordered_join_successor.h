#ifndef SEARCH_ORDERED_JOIN_SUCCESSOR_H
#define SEARCH_ORDERED_JOIN_SUCCESSOR_H

#include "generic_join_successor.h"

/**
 * This class implements the successor generator using a full join ordered by the arity of the predicates.
 * The order is from the predicate with lowest arity to the one with largest.
 *
 */
template <typename OrderT>
class OrderedJoinSuccessorGenerator : public GenericJoinSuccessor {
public:
    explicit OrderedJoinSuccessorGenerator(const Task &task);

  /**
  * @see generic_join_successor.h
  */
  std::vector<Table> parse_precond_into_join_program(
        const std::vector<Atom> &precond,
        const DBState &state) override ;
};


#endif //SEARCH_ORDERED_JOIN_SUCCESSOR_H

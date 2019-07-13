#ifndef SEARCH_NAIVE_SUCCESSOR_H
#define SEARCH_NAIVE_SUCCESSOR_H


#include "successor_generator.h"

class NaiveSuccessorGenerator : public SuccessorGenerator{
public:
    explicit NaiveSuccessorGenerator(const Task &task) : SuccessorGenerator(task) {}

    std::vector<std::pair<State, Action>> generate_successors(const std::vector<ActionSchema> &actions, const State &state,
                                                              const StaticInformation &staticInformation) override;

    std::vector<std::vector<int>> obj_per_type; // position I is a list of object indices of type I

    Table instantiate(const ActionSchema &action, const State &state,
                      const StaticInformation &staticInformation) override;

    std::vector<Table>
    parse_precond_into_join_program(const std::vector<Atom> &precond,
                                    const State &state,
                                    const StaticInformation &staticInformation) override;

};


#endif //SEARCH_NAIVE_SUCCESSOR_H

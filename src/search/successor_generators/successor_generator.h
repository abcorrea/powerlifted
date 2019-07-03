#ifndef SEARCH_SUCCESSOR_GENERATOR_H
#define SEARCH_SUCCESSOR_GENERATOR_H


#include <vector>

#include "../state.h"
#include "../action_schema.h"
#include "../task.h"
#include "../database/table.h"

/*
 * This base class implements a join-successor using the join of all positive preconditions in the
 * action schema.
 */

class SuccessorGenerator {
public:

    explicit SuccessorGenerator(const Task &task) {
        obj_per_type.resize(task.type_names.size());
        for (const Object &obj : task.objects) {
            for (int type : obj.getTypes()) {
                obj_per_type[type].push_back(obj.getIndex());
            }
        }

    }

    std::vector<State> generate_successors(const std::vector<ActionSchema> &actions, const State &state,
                                           const StaticInformation &staticInformation);

private:
    std::vector<std::vector<int>> obj_per_type; // position I is a list of object indices of type I

    Table instantiate(const ActionSchema &action, const State &state,
                                    const StaticInformation &staticInformation);

    std::vector<Table>
    parse_precond_into_join_program(const std::vector<Atom> &precond,
                                    const State &state,
                                    const StaticInformation &staticInformation);

    GroundAtom tuple_to_atom(const std::vector<int> &tuple, const std::vector<int> &indices, const Atom &eff);
};


#endif //SEARCH_SUCCESSOR_GENERATOR_H

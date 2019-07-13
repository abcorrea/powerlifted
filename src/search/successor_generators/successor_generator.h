#ifndef SEARCH_SUCCESSOR_GENERATOR_H
#define SEARCH_SUCCESSOR_GENERATOR_H


#include <vector>

#include "../state.h"
#include "../action_schema.h"
#include "../task.h"
#include "../database/table.h"
#include "../action.h"

#include "../database/join.h"

/*
 * This base class implements a join-successor using the join of all positive preconditions in the
 * action schema.
 *
 *
 * Note that successor generators might change the number of generated states. This happens simply because
 * the order of the tuples produced differs depending on the order of the joins.
 *
 */

class SuccessorGenerator {
public:
    std::vector<std::vector<int>> obj_per_type; // position I is a list of object indices of type I

    explicit SuccessorGenerator(const Task &task) {
        obj_per_type.resize(task.type_names.size());
        for (const Object &obj : task.objects) {
            for (int type : obj.getTypes()) {
                obj_per_type[type].push_back(obj.getIndex());
            }
        }

    }

    virtual std::vector<std::pair<State, Action>> generate_successors(const std::vector<ActionSchema> &actions, const State &state,
                                           const StaticInformation &staticInformation) = 0;

    virtual Table instantiate(const ActionSchema &action, const State &state,
                                    const StaticInformation &staticInformation) = 0;

    virtual std::vector<Table>
    parse_precond_into_join_program(const std::vector<Atom> &precond,
                                    const State &state,
                                    const StaticInformation &staticInformation) = 0;

    GroundAtom tuple_to_atom(const std::vector<int> &tuple, const std::vector<int> &indices, const Atom &eff);
};


#endif //SEARCH_SUCCESSOR_GENERATOR_H

#ifndef SEARCH_SUCCESSOR_GENERATOR_H
#define SEARCH_SUCCESSOR_GENERATOR_H

#include "../action.h"
#include "../action_schema.h"
#include "../states/state.h"
#include "../task.h"

#include "../database/join.h"
#include "../database/table.h"

#include <vector>

/**
 * This base class implements a join-successor using the join of all positive preconditions in the
 * action schema.
 *
 * @attention Note that successor generators might change the number of generated states. This happens simply because
 * the order of the arguments produced differs depending on the order of the joins.
 *
 */

class SuccessorGenerator {
    static bool is_ground_action_applicable(const ActionSchema &action,
                                            const DBState &state,
                                            const StaticInformation &staticInformation);

    void apply_lifted_action_effects(const ActionSchema &action,
                                     const std::vector<int> &tuple,
                                     const std::vector<int> &indices,
                                     std::vector<Relation> &new_relation);

    void apply_ground_action_effects(const ActionSchema &action,
                                     std::vector<Relation> &new_relation) const;

    void apply_nullary_effects(const ActionSchema &action,
                               std::vector<bool> &new_nullary_atoms) const;

public:
    explicit SuccessorGenerator(const Task &task) {
        obj_per_type.resize(task.type_names.size());
        for (const Object &obj : task.objects) {
            for (int type : obj.getTypes()) {
                obj_per_type[type].push_back(obj.getIndex());
            }
        }

    }

    virtual ~SuccessorGenerator() = default;

    // position I is a list of object indices of type I
    std::vector<std::vector<int>> obj_per_type;

    const
    std::vector<std::pair<DBState, LiftedOperatorId>> &generate_successors(const std::vector<
        ActionSchema> &actions,
                                                               const DBState &state,
                                                               const StaticInformation &staticInformation);

    virtual Table instantiate(const ActionSchema &action, const DBState &state,
                              const StaticInformation &staticInformation) = 0;

    virtual std::vector<Table>
    parse_precond_into_join_program(const std::vector<Atom> &precond,
                                    const DBState &state,
                                    const StaticInformation &staticInformation,
                                    int action_index) = 0;

    const GroundAtom &tuple_to_atom(const std::vector<int> &tuple,
                                    const std::vector<int> &indices,
                                    const Atom &eff);

    double get_cyclic_time() const {
        return cyclic_time;
    }

    GroundAtom ground_atom;
    std::vector<std::pair<DBState, LiftedOperatorId>> successors;

protected:
    size_t largest_intermediate_relation = 0;
    double cyclic_time = 0;

};

#endif //SEARCH_SUCCESSOR_GENERATOR_H

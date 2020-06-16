#ifndef SEARCH_SUCCESSOR_GENERATOR_H
#define SEARCH_SUCCESSOR_GENERATOR_H

#include <unordered_set>
#include <vector>

// A few forward declarations :-)
class ActionSchema;
class DBState;
class LiftedOperatorId;
class Task;
class Table;

struct Atom;
struct Relation;
struct TupleHash;

typedef std::vector<int> GroundAtom;
typedef DBState StaticInformation;

/**
 * This base class implements a join-successor using the join of all positive preconditions in the
 * action schema.
 *
 * @attention Note that successor generators might change the number of generated states. This happens simply because
 * the order of the arguments produced differs depending on the order of the joins.
 *
 */

class SuccessorGenerator {

    std::vector<bool> is_predicate_static;

    bool is_ground_action_applicable(const ActionSchema &action,
                                     const DBState &state);

    void apply_lifted_action_effects(const ActionSchema &action,
                                     const std::vector<int> &tuple,
                                     std::vector<Relation> &new_relation);

    void apply_ground_action_effects(const ActionSchema &action,
                                     std::vector<Relation> &new_relation) const;

    void apply_nullary_effects(const ActionSchema &action,
                               std::vector<bool> &new_nullary_atoms) const;

    bool is_trivially_inapplicable(const DBState &state, const ActionSchema &action) const;

    void compute_map_indices_to_table_positions(const Table &instantiations,
                                                std::vector<int> &free_var_indices,
                                                std::vector<int> &map_indices_to_position) const;

public:
    explicit SuccessorGenerator(const Task &task);

    virtual ~SuccessorGenerator() = default;

    // position I is a list of object indices of type I
    std::vector<std::vector<int>> obj_per_type;

    std::vector<LiftedOperatorId> get_applicable_actions(
        const std::vector<ActionSchema> &actions,
        const DBState &state);

    void get_applicable_actions(
        const ActionSchema &action,
        const DBState &state,
        std::vector<LiftedOperatorId>& applicable);

    DBState generate_successor(const LiftedOperatorId &op,
                               const ActionSchema& action,
                               const DBState &state);

    virtual Table instantiate(const ActionSchema &action, const DBState &state) = 0;

    const GroundAtom &tuple_to_atom(const std::vector<int> &tuple, const Atom &eff);

    const std::unordered_set<GroundAtom, TupleHash> &get_tuples_from_static_relation(
        size_t i) const;

    bool is_static(size_t i) {
        return is_predicate_static[i];
    }

    GroundAtom ground_atom;

protected:
    size_t largest_intermediate_relation = 0;
    const StaticInformation& static_information;
    void order_tuple_by_free_variable_order(const std::vector<int> &free_var_indices,
                                            const std::vector<int> &map_indices_to_position,
                                            const std::vector<int> &tuple_with_const,
                                            std::vector<int> &ordered_tuple) const;
};

#endif //SEARCH_SUCCESSOR_GENERATOR_H

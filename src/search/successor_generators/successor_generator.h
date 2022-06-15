#ifndef SEARCH_SUCCESSOR_GENERATOR_H
#define SEARCH_SUCCESSOR_GENERATOR_H

#include <vector>

// A few forward declarations :-)
class ActionSchema;
class DBState;
class LiftedOperatorId;

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

public:
    virtual ~SuccessorGenerator() = default;

    std::vector<std::pair<int, std::vector<int>>> added_atoms;

    /**
     * Compute the instantiations of the given action schema that are applicable in
     * the given state.
     *
     * @param action: The action schema
     * @param state: The state on which we want to compute applicability
     * @return A vector of IDs representing each of them a single applicable
     * instantiation of the action schema.
     */
    virtual std::vector<LiftedOperatorId> get_applicable_actions(
            const ActionSchema &action, const DBState &state) = 0;

    /**
     * Generate the state that results from applying the given action to the given state.
     */
    virtual DBState generate_successor(const LiftedOperatorId &op,
                               const ActionSchema& action,
                               const DBState &state) = 0;

    void add_to_added_atoms(int i, const std::vector<int> & atom) {
        added_atoms.emplace_back(i, atom);
    }

    virtual const std::vector<std::pair<int, std::vector<int>>> &get_added_atoms() const {
        return added_atoms;
    }

};

#endif //SEARCH_SUCCESSOR_GENERATOR_H

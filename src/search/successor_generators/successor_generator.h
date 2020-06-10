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
    std::vector<std::vector<int>> obj_per_type;

public:
    explicit SuccessorGenerator(const Task &task);

    virtual ~SuccessorGenerator() = default;

    virtual void get_applicable_actions(const ActionSchema &action,
                                const DBState &state,
                                std::vector<LiftedOperatorId>& applicable) = 0;

    virtual DBState generate_successors(const LiftedOperatorId &op,
                                const ActionSchema& action,
                                const DBState &state) = 0;

    virtual std::vector<LiftedOperatorId> get_applicable_actions(
        const std::vector<ActionSchema> &actions,
        const DBState &state) = 0;


    virtual Table instantiate(const ActionSchema &action, const DBState &state) = 0;

    bool is_static(size_t i) {
        return is_predicate_static[i];
    }

    GroundAtom ground_atom;

protected:
    size_t largest_intermediate_relation = 0;
    const StaticInformation& static_information;
};

#endif //SEARCH_SUCCESSOR_GENERATOR_H

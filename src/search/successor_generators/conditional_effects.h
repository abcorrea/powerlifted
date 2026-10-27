#ifndef SEARCH_CONDITIONAL_EFFECTS_H
#define SEARCH_CONDITIONAL_EFFECTS_H

#include "../structures.h"

#include <unordered_map>
#include <utility>
#include <vector>

class ActionSchema;
struct ConditionalEffect;
class DBState;
class Task;

/**
 * @brief Evaluates the universally quantified conditional effects of action
 * schemas.
 *
 * The conditions are conjunctive queries over the state in which the action
 * is applied, with the action parameters already bound. They are answered
 * by a backtracking nested-loop join over the positive condition atoms, in
 * an order fixed at construction time (atoms whose variables are all bound
 * become hash lookups). Negated atoms and (in)equalities are checked once
 * all variables are bound (every quantified variable occurs in a positive
 * condition atom).
 */
class ConditionalEffectEvaluator {
    struct CompiledArgument {
        enum Kind { CONSTANT, BOUND, NEW, REPEATED };
        Kind kind;
        // Object index for constants, variable slot otherwise.
        int value;
    };

    struct CompiledAtom {
        int predicate;
        bool negated;
        bool use_static;
        bool fully_bound;
        std::vector<CompiledArgument> arguments;
    };

    struct CompiledConditionalEffect {
        int num_slots;
        std::vector<CompiledAtom> positive;
        std::vector<CompiledAtom> negative;
        std::vector<CompiledAtom> equalities;
        std::vector<int> positive_nullary_condition;
        std::vector<int> negative_nullary_condition;
        std::vector<CompiledAtom> effects;
        std::vector<int> positive_nullary_effects;
        std::vector<int> negative_nullary_effects;
    };

    const DBState &static_information;
    std::vector<bool> is_static;
    std::vector<std::vector<CompiledConditionalEffect>> compiled;

    CompiledConditionalEffect compile(const ActionSchema &action,
                                      const ConditionalEffect &ce) const;

    void match(const CompiledConditionalEffect &ce, std::size_t depth,
               std::vector<int> &binding, const DBState &state,
               std::vector<std::pair<int, GroundAtom>> &deletes,
               std::vector<std::pair<int, GroundAtom>> &adds) const;

    bool check_remaining(const CompiledConditionalEffect &ce,
                         const std::vector<int> &binding,
                         const DBState &state) const;

    static GroundAtom instantiate(const CompiledAtom &atom, const std::vector<int> &binding);

public:
    explicit ConditionalEffectEvaluator(const Task &task);

    /**
     * Compute the effects triggered by the conditional effects of `action`
     * instantiated with `instantiation` (indexed by parameter index) in
     * `state`. Nullary effects are reported with an empty GroundAtom.
     */
    void evaluate(const ActionSchema &action,
                  const std::vector<int> &instantiation,
                  const std::unordered_map<int, int> &fresh_vars,
                  const DBState &state,
                  std::vector<std::pair<int, GroundAtom>> &deletes,
                  std::vector<std::pair<int, GroundAtom>> &adds) const;
};

#endif  // SEARCH_CONDITIONAL_EFFECTS_H

#ifndef SEARCH_GROUNDING_GROUND_TASK_H
#define SEARCH_GROUNDING_GROUND_TASK_H

#include "relaxed_reachability.h"

#include "../structures.h"

#include "../parallel_hashmap/phmap.h"
#include "../utils/hash.h"

#include <memory>
#include <utility>
#include <vector>

class DBState;
class Task;

struct GroundConditionalEffect {
    int op;
    std::vector<int> condition;
    std::vector<int> negated_condition;
    std::vector<int> add;
    std::vector<int> del;
};

struct GroundOperator {
    int schema;
    //! Instantiation of all parameters of the schema.
    std::vector<int> instantiation;
    std::vector<int> pre;
    std::vector<int> negated_pre;
    std::vector<int> add;
    std::vector<int> del;
    int cost;
    //! Indices into GroundTask::conditional_effects.
    std::vector<int> conditional_effects;
};

struct GroundAxiom {
    int head;
    int stratum;
    std::vector<int> body;
    std::vector<int> negated_body;
};

/**
 * @brief Ground (propositional) representation of a task, restricted to the
 * relaxed-reachable part (computed by compute_relaxed_reachability() from
 * the initial state).
 *
 * Facts are the reachable ground atoms of non-static predicates (including
 * derived predicates and nullary predicates). Static atoms do not appear:
 * operators whose static preconditions do not hold are never generated.
 *
 * All fact lists are sorted and duplicate-free.
 */
class GroundTask {
    std::vector<phmap::flat_hash_map<GroundAtom, int, TupleHash>> atom_to_fact;
    std::vector<int> nullary_fact;

    int add_fact(int predicate, const GroundAtom &args);
    bool build(const Task &task, size_t max_instances, size_t max_operators, double max_seconds);

public:
    std::vector<std::pair<int, GroundAtom>> facts;
    std::vector<GroundOperator> operators;
    std::vector<GroundConditionalEffect> conditional_effects;
    std::vector<GroundAxiom> axioms;
    //! Facts true in the initial state (without derived facts).
    std::vector<int> initial_facts;
    //! Positive goal facts.
    std::vector<int> goal;
    //! Facts that must be false in a goal state (negated goal atoms).
    std::vector<int> negated_goal;
    //! Some positive goal atom is not relaxed reachable.
    bool goal_unreachable = false;

    /*
      Set if there are too many ground operators to ground them: the task
      has facts, axioms, initial state, and goal, but no operators. Instead,
      it has the decomposed relaxed program of the action schemas (for the
      heuristic of the lifted search). Its facts are encoded as: f in [0,
      num_facts()) a fact, f < 0 the complement of fact -f - 1, and f >=
      num_facts() the auxiliary fact f - num_facts().
    */
    bool relaxed_program_only = false;
    //! Instance of a rule of the relaxed program, on the encoded facts.
    struct RelaxedProgramOperator {
        int rule;
        std::vector<int> pre;
        std::vector<int> eff;
        std::vector<int> values;
    };
    std::vector<RelaxedProgram::Rule> relaxed_program_rules;
    std::vector<RelaxedProgramOperator> relaxed_program_operators;
    int num_relaxed_program_aux_facts = 0;

    int num_facts() const { return facts.size(); }

    //! Fact id of the given atom, or -1 if it is not a (reachable) fact.
    int get_fact(int predicate, const GroundAtom &args) const
    {
        if (args.empty())
            return nullary_fact[predicate];
        const auto &m = atom_to_fact[predicate];
        auto it = m.find(args);
        return it == m.end() ? -1 : it->second;
    }

    //! Append the fact ids of the atoms true in the state to `out`.
    void get_state_facts(const DBState &state, std::vector<int> &out) const;

    /**
     * Ground the task. Returns nullptr if the task is outside the groundable
     * fragment (object creation), or if grounding takes more than
     * `max_seconds` of wall-clock time or too much memory. If the task has
     * more than `max_operators` ground operators, the result has no
     * operators but the decomposed relaxed program (see relaxed_program_only),
     * or is nullptr if that program is too large.
     */
    static std::shared_ptr<GroundTask>
    create(const Task &task, size_t max_operators, double max_seconds);
};

#endif  // SEARCH_GROUNDING_GROUND_TASK_H

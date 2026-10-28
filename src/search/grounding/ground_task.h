#ifndef SEARCH_GROUNDING_GROUND_TASK_H
#define SEARCH_GROUNDING_GROUND_TASK_H

#include "../structures.h"

#include "../parallel_hashmap/phmap.h"
#include "../utils/hash.h"

#include <memory>
#include <utility>
#include <vector>

class DBState;
class Task;

/**
 * @brief Ground (propositional) representation of a task, restricted to the
 * relaxed-reachable part (computed with the Datalog exploration from the
 * initial state).
 *
 * Facts are the reachable ground atoms of non-static predicates (including
 * derived predicates and nullary predicates). Static atoms do not appear:
 * operators whose static preconditions do not hold are never generated.
 *
 * All fact lists are sorted and duplicate-free.
 */
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

class GroundTask {
    std::vector<phmap::flat_hash_map<GroundAtom, int, TupleHash>> atom_to_fact;
    std::vector<int> nullary_fact;

    int add_fact(int predicate, const GroundAtom &args);
    void renumber_facts();

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
    bool operators_omitted = false;
    struct RelaxationRule {
        int schema;
        int cost;
        bool final;
        //! Variables bound by an instance (schema parameters below the
        //! schema's number of parameters).
        std::vector<int> vars;
    };
    struct RelaxationOperator {
        int rule;
        std::vector<int> pre;
        std::vector<int> eff;
        std::vector<int> values;
    };
    std::vector<RelaxationRule> relaxation_rules;
    std::vector<RelaxationOperator> relaxation_operators;
    int num_relaxation_aux = 0;

    /**
     * Ground the task. Returns false if the grounding exceeds the given
     * limits (number of Datalog facts produced by the exploration, number
     * of ground operators); the object is then unusable.
     */
    bool build(const Task &task, size_t max_datalog_facts, size_t max_operators);

    int num_facts() const {
        return facts.size();
    }

    //! Fact id of the given atom, or -1 if it is not a (reachable) fact.
    int get_fact(int predicate, const GroundAtom &args) const {
        if (args.empty())
            return nullary_fact[predicate];
        const auto &m = atom_to_fact[predicate];
        auto it = m.find(args);
        return it == m.end() ? -1 : it->second;
    }

    //! Append the fact ids of the atoms true in the state to `out`.
    void get_state_facts(const DBState &state, std::vector<int> &out) const;

    /**
     * Ground the task once and cache the result. Returns nullptr if the task
     * is outside the groundable fragment (object creation) or too large to
     * ground (or if grounding is disabled with PWL_NO_GROUNDING).
     */
    static std::shared_ptr<GroundTask> get_cached(const Task &task);

    /**
     * If the task has too many ground operators, the task with facts and the
     * relaxed program but without operators (see operators_omitted);
     * nullptr otherwise.
     */
    static std::shared_ptr<GroundTask> get_cached_relaxation(const Task &task);

    //! Make get_cached() return nullptr (lifted planning only).
    static void disable_grounding();
};

#endif  // SEARCH_GROUNDING_GROUND_TASK_H

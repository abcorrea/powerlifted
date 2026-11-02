#ifndef SEARCH_HEURISTICS_GROUND_DELETE_RELAXATION_HEURISTIC_H
#define SEARCH_HEURISTICS_GROUND_DELETE_RELAXATION_HEURISTIC_H

#include "heuristic.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

class GroundTask;

/**
 * @brief Delete-relaxation heuristics (h-add, h-max, h-FF) computed on the
 * ground representation of the task.
 *
 * The relaxed operators are the ground operators, the ground conditional
 * effects (precondition plus effect condition), and the ground axioms (cost
 * 0). Costs are propagated with a generalized Dijkstra exploration over a
 * bucket queue; h-FF extracts a relaxed plan from the best supporters and
 * counts every ground action once. Like the Datalog-based heuristics, the
 * heuristic exposes the relaxed plan's atoms that are false in the
 * evaluated state as useful atoms.
 */
class GroundDeleteRelaxationHeuristic : public Heuristic {
public:
    enum Type { ADD, MAX, FF };

    //! Type of the heuristic with the given name (add, hmax, or ff, ignoring
    //! case), if any.
    static std::optional<Type> get_type(const std::string &name);

    /*
      Action of a relaxed operator that is not (part of) a ground operator:
      none (axioms and auxiliary rules of decompositions), a final rule of
      the decomposition of a ground action schema (its ground operator is
      reconstructed from the supporters), or a final rule of the relaxed
      program of a task without ground operators.
    */
    static const int NO_ACTION = -1;
    static const int DECOMPOSED_ACTION = -2;
    static const int RELAXED_PROGRAM_ACTION = -3;

private:
    Type type;
    std::shared_ptr<GroundTask> ground_task;
    //! Facts of the ground task plus complement facts: for every fact f
    //! occurring negated in a precondition, effect condition, or the goal,
    //! a fact not-f (true iff f is false, added by the operators deleting f).
    int num_facts;
    int num_base_facts;
    std::vector<int> complement;
    std::vector<int> complement_base;

    // Relaxed operators in CSR form.
    std::vector<int> op_pre_start;
    std::vector<int> op_pre;
    std::vector<int> op_eff_start;
    std::vector<int> op_eff;
    std::vector<int> op_base_cost;
    std::vector<int> op_num_pre;
    //! Ground operator the relaxed operator belongs to, or one of the
    //! values NO_ACTION, DECOMPOSED_ACTION, RELAXED_PROGRAM_ACTION.
    std::vector<int> op_action;
    std::vector<int> op_action_cost;
    std::vector<int> no_pre_ops;
    // For each fact, the relaxed operators having it as precondition.
    std::vector<int> pre_of_start;
    std::vector<int> pre_of;
    // For each fact, the relaxed operators adding it.
    std::vector<int> achiever_start;
    std::vector<int> achievers;
    std::vector<int> goal;
    bool goal_unreachable;

    // Per-evaluation data (valid if the stamp equals the generation).
    struct FactState {
        int stamp = 0;
        int cost = 0;
        int supporter = -1;
    };
    // Hot per-operator data of the exploration (the static data is read
    // from op_base_cost, op_num_pre, and op_eff_start).
    struct RelaxedOp {
        int stamp = 0;
        int unsatisfied = 0;
        int cost = 0;
        int num_pre = 0;
    };
    std::vector<FactState> fact_state;
    std::vector<RelaxedOp> relaxed_ops;
    int generation = 0;
    std::vector<std::vector<int>> buckets;
    //! Heap of (cost, fact) for costs beyond the bucket queue.
    std::vector<std::pair<int, int>> overflow;
    std::vector<int> state_facts;
    std::vector<char> in_state;
    std::vector<char> marked_fact;
    std::vector<char> marked_action;
    std::vector<int> marked_actions_list;
    std::vector<int> marked_facts_list;
    std::vector<int> stack;
    std::vector<int> relaxed_plan;
    std::vector<int> relaxed_plan_levels;
    std::vector<int> useful_facts;

    //! Ground actions that can contribute to reaching the goal.
    std::vector<char> relevant_action;

    /*
      Join decomposition of the relaxed operators of large action schemas
      (see add_decomposed_operators()). Facts from first_aux_fact on are
      auxiliary facts of the decomposition.
    */
    struct DecomposedRule {
        //! Variables (schema parameters) bound by an instance.
        std::vector<int> vars;
        //! For final rules, the operator index to find the ground operator
        //! of an instance; -1 for auxiliary rules.
        int op_index;
        //! Schema and its number of parameters (relaxed program of a task
        //! without ground operators).
        int schema = -1;
        int num_params = 0;
    };
    struct OperatorIndex {
        int schema;
        std::vector<int> vars;
        //! Ground operators of the schema, sorted by their values on vars.
        std::vector<int> ops;
    };
    std::vector<DecomposedRule> decomposed_rules;
    std::vector<OperatorIndex> operator_indices;
    int first_aux_fact;
    //! Per relaxed operator: its decomposed rule (or -1) and the offset of
    //! the values of the rule's variables in rule_values.
    std::vector<int> op_rule;
    std::vector<int> op_values_start;
    std::vector<int> rule_values;
    std::vector<int> binding;
    std::vector<int> reconstruct_stack;
    //! Ground operators of the decomposed schemas.
    std::vector<int> decomposed_operators;
    //! Actions (schema, parameters) of the last relaxed plan of a task
    //! without ground operators.
    std::vector<std::vector<int>> lifted_plan_actions;
    void add_relaxed_program_operators(const Task &task);
    //! Mark the relaxed plan part of the relaxed program operator r (no
    //! ground operators): push its precondition facts and those of the
    //! supporters of its auxiliary facts; return the action's cost if the
    //! action is new in the relaxed plan, 0 otherwise.
    int extract_relaxed_program_action(int r);

    int add_relaxed_op(const std::vector<int> &pre1,
                       const std::vector<int> *pre2,
                       const std::vector<int> &eff,
                       int base_cost,
                       int action);
    void add_decomposed_operators(const Task &task, std::vector<char> &decomposed);
    //! Ground operator of the (closed) decomposed final relaxed operator r,
    //! following the supporters of its auxiliary preconditions; -1 if none.
    int reconstruct_operator(int r);
    void push(int fact, int cost);
    int explore();
    void prune_irrelevant(bool some_negation_relaxed);

public:
    GroundDeleteRelaxationHeuristic(const Task &task,
                                    std::shared_ptr<GroundTask> ground_task,
                                    Type type);

    int compute_heuristic(const DBState &s, const Task &task) override;

    /**
     * Compute the heuristic for the state whose true facts are `facts`. If
     * `fill_useful_atoms` is set, the useful atoms (lifted representation)
     * are filled as well; the useful facts (fact ids of the relaxed plan's
     * atoms false in the state) are always available afterwards (h-FF).
     */
    int compute_for_facts(const std::vector<int> &facts, bool fill_useful_atoms);

    //! Facts of the last relaxed plan that are false in the evaluated
    //! state (h-FF). Ids >= get_num_base_facts() are complement facts.
    const std::vector<int> &get_useful_facts() const { return useful_facts; }

    //! Ground actions that are relevant for the goal (all actions if the
    //! analysis cannot rule out any).
    const std::vector<char> &get_relevant_actions() const { return relevant_action; }

    int get_num_base_facts() const { return num_base_facts; }

    //! Base fact of complement fact c (c >= get_num_base_facts()).
    int get_complement_base(int c) const { return complement_base[c - num_base_facts]; }

    // Relaxed operators (CSR form), used by the landmark generator.
    int get_num_facts() const { return num_facts; }
    //! Facts below this are base and complement facts; the others are
    //! auxiliary facts of decomposed relaxed operators.
    int get_first_aux_fact() const { return first_aux_fact; }
    int get_num_relaxed_ops() const { return op_base_cost.size(); }
    const std::vector<int> &get_op_pre_start() const { return op_pre_start; }
    const std::vector<int> &get_op_pre() const { return op_pre; }
    const std::vector<int> &get_op_eff_start() const { return op_eff_start; }
    const std::vector<int> &get_op_eff() const { return op_eff; }
    const std::vector<int> &get_op_action() const { return op_action; }
    const std::vector<int> &get_pre_of_start() const { return pre_of_start; }
    const std::vector<int> &get_pre_of() const { return pre_of; }
    const std::vector<int> &get_no_pre_ops() const { return no_pre_ops; }
    //! Goal facts of the relaxed task (including complement facts).
    const std::vector<int> &get_goal() const { return goal; }
    //! Ground operators whose relaxed operators were decomposed (their
    //! decomposed final operators have action -2).
    const std::vector<int> &get_decomposed_operators() const { return decomposed_operators; }
    //! Precondition and effect of the relaxed version of ground operator o.
    void get_ground_relaxed_operator(int o, std::vector<int> &pre, std::vector<int> &eff) const;

    //! Append the complement facts true in the state with the given true
    //! base facts.
    void add_complement_facts(const std::vector<int> &facts, std::vector<int> &out);

    //! Ground operators of the last relaxed plan computed (h-FF only).
    const std::vector<int> &get_relaxed_plan() const { return relaxed_plan; }

    //! For each operator of the relaxed plan, the h-add cost of the fact
    //! it was selected for (a layer for ordering the relaxed plan).
    const std::vector<int> &get_relaxed_plan_levels() const { return relaxed_plan_levels; }
};

#endif  // SEARCH_HEURISTICS_GROUND_DELETE_RELAXATION_HEURISTIC_H

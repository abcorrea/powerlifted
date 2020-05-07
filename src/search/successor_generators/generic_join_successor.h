#ifndef SEARCH_GENERIC_JOIN_SUCCESSOR_H
#define SEARCH_GENERIC_JOIN_SUCCESSOR_H

#include "successor_generator.h"
#include "../structures.h"

#include <map>
#include <set>

class PrecompiledActionData;

/**
 * This class is not a successor generator per se. It just contain most of the common functions
 * used over all the join successor generators.
 *
 * @details This contains the main functions for successor generators based on
 * join of preconditions .The main function of this class is the instantiate
 * function, which performs the join program itself. If used, this class
 * orders the join program using the same order as the PDDL file. This behavior
 * is defined in the function parse_precond_into_join_program. Classes
 * extending the GenericJoinSuccessor class usually replace this function for
 * something more elaborated.
 *
 * @see database/join.cc
 */
class GenericJoinSuccessor : public SuccessorGenerator {

public:
    explicit GenericJoinSuccessor(const Task &task);

    Table instantiate(const ActionSchema &action, const DBState &state) override;

    /**
    * Create the set of tables corresponding to the precondition of the given action.
    *
    * We first obtain all indices in the precondition that are constants.
    * Then, we create the table applying the projection over the arguments
    * that satisfy the instantiation of the constants. There are two cases
    * for the projection:
    *    1. The table comes from the static information; or
    *    2. The table comes directly from the current state.
    *
    * @param adata: A set of relevant data corresponding to the action in question
    * @param state: state being evaluated
    * @param tables: the set of tables, output parameter.
    * @return false if some table is empty and hence the action inapplicable, true otherwise.
    */
    virtual bool parse_precond_into_join_program(const PrecompiledActionData &adata,
                                                       const DBState &state,
                                                       std::vector<Table>& tables);

protected:
    static void get_indices_and_constants_in_preconditions(std::vector<int> &indices,
                                                           std::vector<int> &constants,
                                                           const Atom &a);

    static void select_tuples(const DBState &s,
                              const Atom &a,
                              std::vector<GroundAtom> &tuples,
                              const std::vector<int> &constants);

    void filter_inequalities(const ActionSchema &action,
                             Table &working_table) const;
    static void create_hypergraph(
        const ActionSchema &action,
        std::vector<int> &hypernodes,
        std::vector<std::set<int>> &hyperedges,
        std::vector<int> &missing_precond,
        std::map<int, int> &node_index,
        std::map<int, int> &node_counter,
        std::map<int, int> &edge_to_precond);

    std::vector<PrecompiledActionData> precompile_action_data(
        const std::vector<ActionSchema>& actions);

    PrecompiledActionData precompile_action_data(const ActionSchema& action);

    //! Some data relevant to each action schema, indexed by schema index
    std::vector<PrecompiledActionData> action_data;
};

class PrecompiledActionData {
public:
    //! Whether the action has no parameters
    bool is_ground;

    //! Whether the schema is statically inapplicable
    bool statically_inapplicable;

    std::vector<Atom> relevant_precondition_atoms;

    //! A list of the indexes in `relevant_precondition_atoms` that correspond to fluent atoms,
    //! and hence their tables need to be created for each state.
    std::vector<unsigned> fluent_tables;

    //! A set of tables with all static info precompiled for faster access at runtime
    std::vector<Table> precompiled_db;
};

#endif //SEARCH_GENERIC_JOIN_SUCCESSOR_H

#include <iostream>
#include <cassert>
#include <vector>
#include <queue>

#include "successor_generator.h"
#include "naive_successor.h"


using namespace std;

vector<pair<State, Action>> NaiveSuccessorGenerator::generate_successors(const vector<ActionSchema> &actions,
                                                                         const State &state,
                                                                         const StaticInformation &staticInformation) {
    vector<pair<State, Action>> successors;

    for (const ActionSchema &action : actions) {
        Table instantiations = instantiate(action, state, staticInformation);
        /*
         * Two cases:
         *    - At least one instantiation found:
         *      Instantiate the schema for every tuple, check the negative precond., and generate successor.
         *    - No instantiation found.
         *      Then it depends whether the action has parameters or not.
         *      Right now, we are only considering actions with parameters, and then there is really no instantiation
         *      possible for this action schema in this state.
         *
         *  IMPORTANT:
         *    - We are not supporting negative precond right now, hence we are simply generating the succ
         *      (In practice, we must only support != but it is not sure where to check that)
         */
        if (instantiations.tuples.empty()) {
            continue;
            // TODO case where action is pre grounded (no parameters)
        } else {
            for (const vector<int> &tuple : instantiations.tuples) {
                // TODO support for !=
                // TODO test case with constants (should work?)
                vector<Relation> new_relation(state.relations);
                for (const Atom &eff : action.getEffects()) {
                    GroundAtom ground_atom = tuple_to_atom(tuple, instantiations.tuple_index, eff);
                    assert (eff.predicate_symbol == new_relation[eff.predicate_symbol].predicate_symbol);
                    if (eff.negated) {
                        // Remove from relation
                        new_relation[eff.predicate_symbol].tuples.erase(remove(
                                new_relation[eff.predicate_symbol].tuples.begin(),
                                new_relation[eff.predicate_symbol].tuples.end(), ground_atom),
                                                                        new_relation[eff.predicate_symbol].tuples.end());
                    } else {
                        if (find(new_relation[eff.predicate_symbol].tuples.begin(),
                                 new_relation[eff.predicate_symbol].tuples.end(), ground_atom)
                            == new_relation[eff.predicate_symbol].tuples.end()) {
                            // If ground atom is not in the state, we add it
                            new_relation[eff.predicate_symbol].tuples.push_back(ground_atom);
                        }
                    }
                }
                successors.emplace_back(new_relation, Action(action.getIndex(), tuple));
            }
        }
    }
    return successors;
}

Table NaiveSuccessorGenerator::instantiate(const ActionSchema &action, const State &state,
                                           const StaticInformation &staticInformation) {
    /*
     * We first certify that there are preconditions for the action.
     *
     * IMPORTANT:
     *    - We only perform the join over the POSITIVE preconditions, NEGATIVE preconditions
     *      are not supported by now
     */
    vector<vector<int>> instantiations;
    const vector<Parameter> &params = action.getParameters();
    vector<Atom> precond;
    for (const Atom &p : action.getPrecondition()) {
        // Ignoring negative preconditions when instantiating
        if (not p.negated) {
            precond.push_back((p));
        }
    }

    if (params.empty()) {
        return Table();
    }

    assert (!precond.empty());

    vector<Table> tables = parse_precond_into_join_program(precond, state, staticInformation);
    assert (!tables.empty());
    Table working_table = tables[0];
    for (int i = 1; i < tables.size(); ++i) {
        join(working_table, tables[i]);
        // Filter out equalities
        for (const pair<int, int> ineq : action.getInequalities()) {
            auto it_1 = find(working_table.tuple_index.begin(),
                             working_table.tuple_index.end(),
                             ineq.first);
            auto it_2 = find(working_table.tuple_index.begin(),
                             working_table.tuple_index.end(),
                             ineq.second);
            if (it_1 != working_table.tuple_index.end() and it_2 != working_table.tuple_index.end()) {
                // Loop over all tuples and remove duplicates
                // This is a late removal but there is no easy way to do it before
                int index1 = distance(working_table.tuple_index.begin(), it_1);
                int index2 = distance(working_table.tuple_index.begin(), it_2);
                int cont = 0;
                vector<int> equal_tuples;
                for (vector<int> tuple : working_table.tuples) {
                    if (tuple[index1] == tuple[index2]) {
                        equal_tuples.push_back(cont);
                    }
                    cont++;
                }
                reverse(equal_tuples.begin(), equal_tuples.end());
                for (int n : equal_tuples) {
                    working_table.tuples.erase(working_table.tuples.begin() + n);
                }
            }
        }
        if (working_table.tuples.empty()) {
            return working_table;
        }
    }

    return working_table;
}

vector<Table> NaiveSuccessorGenerator::parse_precond_into_join_program(const vector<Atom> &precond, const State &state,
                                                                       const StaticInformation &staticInformation) {
    /*
     * We first parse the state and the atom preconditions into a set of tables
     * to perform the join-program more easily.
     */
    vector<Table> parsed_tables;//(precond.size());
    parsed_tables.reserve(precond.size());
    for (const Atom &a : precond) {
        vector<int> indices;
        for (Argument arg : a.tuples) {
            indices.push_back(arg.index);
        }
        if (!staticInformation.relations[a.predicate_symbol].tuples.empty()) {
            // If this predicate has information in the static information table,
            // then it must be a static predicate
            parsed_tables.emplace_back(staticInformation.relations[a.predicate_symbol].tuples, indices);
        } else {
            // If this predicate does not have information in the static information table,
            // then it must be a fluent
            parsed_tables.emplace_back(state.relations[a.predicate_symbol].tuples, indices);
        }
    }
    return parsed_tables;
}

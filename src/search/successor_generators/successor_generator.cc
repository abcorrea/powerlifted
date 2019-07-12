#include <iostream>
#include <cassert>
#include <vector>

#include "successor_generator.h"

#include "../database/join.h"
#include "../database/table.h"
#include "../action.h"

using namespace std;

vector<pair<State,Action>> SuccessorGenerator::generate_successors(const vector<ActionSchema> &actions,
                                                      const State &state, const StaticInformation &staticInformation) {
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
        }
        else {
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
                    }
                    else {
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

Table SuccessorGenerator::instantiate(const ActionSchema &action, const State &state,
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
    for (const Atom& p : action.getPrecondition()) {
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
        if (working_table.tuples.empty()) {
            return working_table;
        }
    }

    return working_table;
}

vector<Table> SuccessorGenerator::parse_precond_into_join_program(const vector<Atom> &precond, const State &state,
                                                                  const StaticInformation &staticInformation) {
    /*
     * We first parse the state and the atom preconditions into a set of tables
     * to perform the join-program more easily.
     */
    vector<Table> parsed_tables;//(precond.size());
    parsed_tables.reserve(precond.size());
    for (const Atom& a : precond) {
        vector<int> indices;
        for (Argument arg : a.tuples) {
            indices.push_back(arg.index);
        }
        if (!staticInformation.relations[a.predicate_symbol].tuples.empty()) {
            // If this predicate has information in the static information table,
            // then it must be a static predicate
            parsed_tables.emplace_back(staticInformation.relations[a.predicate_symbol].tuples, indices);
        }
        else {
            // If this predicate does not have information in the static information table,
            // then it must be a fluent
            parsed_tables.emplace_back(state.relations[a.predicate_symbol].tuples, indices);
        }
    }
    return parsed_tables;
}

GroundAtom SuccessorGenerator::tuple_to_atom(const vector<int> &tuple, const vector<int> &indices, const Atom &eff) {
    vector<int> ordered_tuple(tuple.size(), -1);
    assert (tuple.size() == indices.size());
    for (int i = 0; i < indices.size(); ++i) {
        ordered_tuple[indices[i]] = tuple[i];
    }

    GroundAtom ground_atom(eff.tuples.size(), -1);
    for (int i = 0; i < ground_atom.size(); i++) {
        ground_atom[i] = ordered_tuple[eff.tuples[i].index];
    }

    //Sanity check
    for (int v : ground_atom) {
        assert (v != -1);
    }

    return ground_atom;
}



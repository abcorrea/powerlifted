#include <iostream>
#include <cassert>
#include <vector>
#include <queue>


#include "generic_join_successor.h"

#include "../database/hash_join.h"

const vector<pair<State, Action>> &GenericJoinSuccessor::generate_successors(
        const vector<ActionSchema> &actions,
        const State &state,
        const StaticInformation &staticInformation) {
    successors.clear();

    for (const ActionSchema &action : actions) {
        cout << "Generating instantiation of action " << action.getName() << endl;
        bool trivially_inapplicable = false;
        for (int i = 0; i < action.positive_nullary_precond.size() and !trivially_inapplicable; ++i) {
            if ((action.positive_nullary_precond[i] and !state.nullary_atoms[i])
                or (action.negative_nullary_precond[i] and state.nullary_atoms[i])) {
                trivially_inapplicable = true;
            }
        }
        if (trivially_inapplicable) {
            continue;
        }
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
            if (action.getParameters().empty()) {
                bool applicable = true;
                for (const Atom& precond : action.getPrecondition()) {
                    int index = precond.predicate_symbol;
                    vector<int> tuple;
                    tuple.reserve(precond.tuples.size());
                    for (const Argument &arg : precond.tuples) {
                        assert(arg.constant);
                        tuple.push_back(arg.index); // Index of a constant is the obj index
                    }
                    if (!state.relations[index].tuples.empty()) {
                        if (precond.negated) {
                            if (state.relations[index].tuples.find(tuple) != state.relations[index].tuples.end())
                                applicable = false;
                        }
                        else {
                            if (state.relations[index].tuples.find(tuple) == state.relations[index].tuples.end())
                                applicable = false;
                        }
                    }
                    if (!staticInformation.relations[index].tuples.empty()) {
                        if (precond.negated) {
                            if (staticInformation.relations[index].tuples.find(tuple)
                                != staticInformation.relations[index].tuples.end())
                                applicable = false;
                        }
                        else {
                            if (staticInformation.relations[index].tuples.find(tuple)
                                == staticInformation.relations[index].tuples.end())
                                applicable = false;
                        }
                    }
                }

                if (!applicable)
                    continue;

                vector<bool> new_nullary_atoms(state.nullary_atoms);
                for (int i = 0; i < action.negative_nullary_effects.size(); ++i) {
                    if (action.negative_nullary_effects[i])
                        new_nullary_atoms[i] = false;
                }
                for (int i = 0; i < action.positive_nullary_effects.size(); ++i) {
                    if (action.positive_nullary_effects[i])
                        new_nullary_atoms[i] = true;
                }

                vector<Relation> new_relation(state.relations);
                for (const Atom &eff : action.getEffects()) {
                    GroundAtom groundAtom;
                    groundAtom.reserve(eff.tuples.size());
                    for (auto t : eff.tuples) {
                        assert(t.constant);
                        groundAtom.push_back(t.index);
                    }
                    assert (eff.predicate_symbol == new_relation[eff.predicate_symbol].predicate_symbol);
                    if (eff.negated) {
                        // Remove from relation
                        new_relation[eff.predicate_symbol].tuples.erase(ground_atom);
                    } else {
                        if (find(new_relation[eff.predicate_symbol].tuples.begin(),
                                 new_relation[eff.predicate_symbol].tuples.end(), ground_atom)
                            == new_relation[eff.predicate_symbol].tuples.end()) {
                            // If ground atom is not in the state, we add it
                            new_relation[eff.predicate_symbol].tuples.insert(ground_atom);
                        }
                    }
                }
                successors.emplace_back(State(new_relation, new_nullary_atoms),
                                        Action(action.getIndex(), vector<int>()));
            }
            else {
                // Action not applicable
                continue;
            }
        } else {
            vector<bool> new_nullary_atoms(state.nullary_atoms);
            for (int i = 0; i < action.negative_nullary_effects.size(); ++i) {
                if (action.negative_nullary_effects[i])
                    new_nullary_atoms[i] = false;
            }
            for (int i = 0; i < action.positive_nullary_effects.size(); ++i) {
                if (action.positive_nullary_effects[i])
                    new_nullary_atoms[i] = true;
            }
            for (const vector<int> &tuple : instantiations.tuples) {
                // TODO test case with constants (should work?)
                vector<int> ordered_tuple(tuple.size());
                assert(ordered_tuple.size() == instantiations.tuple_index.size());
                for (int i = 0; i < instantiations.tuple_index.size(); ++i) {
                    ordered_tuple[instantiations.tuple_index[i]] = tuple[i];
                }
                vector<Relation> new_relation(state.relations);
                for (const Atom &eff : action.getEffects()) {
                    const GroundAtom &ground_atom = tuple_to_atom(tuple, instantiations.tuple_index, eff);
                    assert (eff.predicate_symbol == new_relation[eff.predicate_symbol].predicate_symbol);
                    if (eff.negated) {
                        // Remove from relation
                        new_relation[eff.predicate_symbol].tuples.erase(ground_atom);
                    } else {
                        if (find(new_relation[eff.predicate_symbol].tuples.begin(),
                                 new_relation[eff.predicate_symbol].tuples.end(), ground_atom)
                            == new_relation[eff.predicate_symbol].tuples.end()) {
                            // If ground atom is not in the state, we add it
                            new_relation[eff.predicate_symbol].tuples.insert(ground_atom);
                        }
                    }
                }
                successors.emplace_back(State(new_relation, new_nullary_atoms),
                        Action(action.getIndex(), ordered_tuple));
            }
        }
    }
    return successors;
}

Table GenericJoinSuccessor::instantiate(const ActionSchema &action, const State &state,
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

    if (params.empty()) {
        return Table();
    }


    vector<Atom> precond;
    for (const Atom &p : action.getPrecondition()) {
        // Ignoring negative preconditions when instantiating
        if ((!p.negated) and p.tuples.size() > 0) {
            precond.push_back((p));
        }
    }

    assert (!precond.empty());

    vector<Table> tables = parse_precond_into_join_program(precond, state, staticInformation, action.getIndex());
    assert (!tables.empty());
    Table &working_table = tables[0];
    for (int i = 1; i < tables.size(); ++i) {
        hash_join(working_table, tables[i]);
        // Filter out equalities
        for (const pair<int, int> ineq : action.getInequalities()) {
            auto it_1 = find(working_table.tuple_index.begin(),
                             working_table.tuple_index.end(),
                             ineq.first);
            auto it_2 = find(working_table.tuple_index.begin(),
                             working_table.tuple_index.end(),
                             ineq.second);
            int index1 = distance(working_table.tuple_index.begin(), it_1);
            int index2 = distance(working_table.tuple_index.begin(), it_2);
            if (it_1 != working_table.tuple_index.end() and it_2 != working_table.tuple_index.end()) {
                vector<vector<int>> to_remove;
                for (auto && t : working_table.tuples) {
                    if (t[index1] == t[index2])
                        to_remove.push_back(t);
                }
                for (auto &&t : to_remove) {
                    working_table.tuples.erase(t);
                }
            }
        }
        if (working_table.tuples.empty()) {
            return working_table;
        }
    }

    return working_table;
}

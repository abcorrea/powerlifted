#include <iostream>
#include <cassert>
#include <vector>
#include <queue>

#include "successor_generator.h"
#include "../action.h"

using namespace std;

const std::vector<std::pair<State, Action>>
&SuccessorGenerator::generate_successors(const std::vector<ActionSchema> &actions, const State &state,
                                         const StaticInformation &staticInformation) {

    successors.clear();
    // Duplicate code from generic join implementation
    for (const ActionSchema &action : actions) {
        //cout << "Generating instantiation of action " << action.getName() << endl;
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
        //cout << "Instantiating action " << action.getName() << endl;
        Table instantiations = instantiate(action, state, staticInformation);
        /*
         * See comment in generic_join_successor.cc
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
                    else if (!staticInformation.relations[index].tuples.empty()) {
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
                    else {
                        applicable = false;
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
                    GroundAtom ga;
                    for (const Argument &a : eff.tuples) {
                        assert(a.constant);
                        ga.push_back(a.index);
                    }
                    assert (eff.predicate_symbol == new_relation[eff.predicate_symbol].predicate_symbol);
                    if (eff.negated) {
                        // Remove from relation
                        new_relation[eff.predicate_symbol].tuples.erase(ga);
                    } else {
                        // If ground atom is not in the state, we add it
                        new_relation[eff.predicate_symbol].tuples.insert(ga);
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
            for (const vector<int> &tuple_with_const : instantiations.tuples) {
                // First, order tuple of indices and then apply effects
                vector<int> tuple;
                vector<int> indices;
                for (int j = 0; j < instantiations.tuple_index.size(); ++j) {
                    if (instantiations.tuple_index[j] >= 0) {
                        indices.push_back(instantiations.tuple_index[j]);
                        tuple.push_back(tuple_with_const[j]);
                    }
                }
                vector<int> ordered_tuple(tuple.size());
                assert(ordered_tuple.size() == indices.size());
                for (int i = 0; i < indices.size(); ++i) {
                    ordered_tuple[indices[i]] = tuple[i];
                }
                vector<Relation> new_relation(state.relations);
                for (const Atom &eff : action.getEffects()) {
                    const GroundAtom &ga = tuple_to_atom(tuple, indices, eff);
                    assert (eff.predicate_symbol == new_relation[eff.predicate_symbol].predicate_symbol);
                    if (eff.negated) {
                        // Remove from relation
                        new_relation[eff.predicate_symbol].tuples.erase(ga);
                    } else {
                        if (find(new_relation[eff.predicate_symbol].tuples.begin(),
                                 new_relation[eff.predicate_symbol].tuples.end(), ga)
                            == new_relation[eff.predicate_symbol].tuples.end()) {
                            // If ground atom is not in the state, we add it
                            new_relation[eff.predicate_symbol].tuples.insert(ga);
                        }
                    }
                }
                successors.emplace_back(State(new_relation, new_nullary_atoms),
                                        Action(action.getIndex(), ordered_tuple));
            }
        }
    }
    //cout << "Largest intermediate relation: " << largest_intermediate_relation << endl;
    return successors;
}

const GroundAtom &SuccessorGenerator::tuple_to_atom(const vector<int> &tuple,
                                                    const vector<int> &indices,
                                                    const Atom &eff) {


    /*
     *    This action generates the ground atom produced by an atomic effect given an instantiation of
     *    its parameters.
     *
     *    First, we rearrange the indices. Then, we create the atom based on whether the argument
     *    is a constant or not. If it is, then we simply pass the constant value; otherwise we use
     *    the instantiation that we found.
     */

    vector<int> ordered_tuple(tuple.size(), -1);
    assert (tuple.size() == indices.size());
    for (int i = 0; i < indices.size(); ++i) {
        ordered_tuple[indices[i]] = tuple[i];
    }

    ground_atom.clear();
    ground_atom.reserve(eff.tuples.size());
    for (int i = 0; i < eff.tuples.size(); i++) {
        if (!eff.tuples[i].constant)
            ground_atom.push_back(ordered_tuple[eff.tuples[i].index]);
        else
            ground_atom.push_back(eff.tuples[i].index);
    }

    //Sanity check
    for (int v : ground_atom) {
        assert (v != -1);
    }

    return ground_atom;
}

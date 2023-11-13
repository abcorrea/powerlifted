#include "clique_bron_kerbosch.h"
#include "clique_help_functions.h"
#include "clique_successor_generator.h"

#include <cassert>
#include <limits>
#include <unordered_map>
#include <vector>

using namespace std;

/*
 * The code for applying actions is copied from generic_join_successor.cc
 */

void CliqueSuccessorGenerator::apply_nullary_effects(const ActionSchema &action,
                                                     vector<bool> &new_nullary_atoms) {
    /*
     * Loop over positive and negative nullary effects and apply them accordingly
     * to the state.
     */
    for (size_t i = 0; i < action.get_negative_nullary_effects().size(); ++i) {
        if (action.get_negative_nullary_effects()[i]) {
            new_nullary_atoms[i] = false;
        }
    }

    for (size_t i = 0; i < action.get_positive_nullary_effects().size(); ++i) {
        if (action.get_positive_nullary_effects()[i]) {
            new_nullary_atoms[i] = true;
            add_to_added_atoms(i, GroundAtom());
        }
    }
}

void CliqueSuccessorGenerator::apply_ground_action_effects(const ActionSchema &action,
                                                           vector<Relation> &new_relation) {
    for (const Atom &eff : action.get_effects()) {
        GroundAtom ga;
        for (const Argument &a : eff.get_arguments()) {
            // Create ground atom for each effect given the instantiation
            assert(a.is_constant());
            ga.push_back(a.get_index());
        }

        assert(eff.get_predicate_symbol_idx() ==
               new_relation[eff.get_predicate_symbol_idx()].predicate_symbol);

        if (eff.is_negated()) {
            // If ground effect is negated, remove it from relation
            new_relation[eff.get_predicate_symbol_idx()].tuples.erase(ga);
        }
        else {
            // If ground effect is not in the state, we add it

            new_relation[eff.get_predicate_symbol_idx()].tuples.insert(ga);
            add_to_added_atoms(eff.get_predicate_symbol_idx(), ga);
        }
    }
}

const GroundAtom CliqueSuccessorGenerator::tuple_to_atom(const vector<int> &tuple, const Atom &eff) {
    GroundAtom ground_atom;
    ground_atom.reserve(eff.get_arguments().size());
    for (auto argument : eff.get_arguments()) {
        if (!argument.is_constant()) {
            ground_atom.push_back(tuple[argument.get_index()]);
        }
        else {
            ground_atom.push_back(argument.get_index());
        }
    }

    // Sanity check: check that all positions of the tuple were initialized
    assert(find(ground_atom.begin(), ground_atom.end(), -1) == ground_atom.end());

    return ground_atom;
}

void CliqueSuccessorGenerator::apply_lifted_action_effects(const ActionSchema &action,
                                                           const vector<int> &tuple,
                                                           vector<Relation> &new_relation) {
    for (const Atom &eff : action.get_effects()) {
        GroundAtom ga = tuple_to_atom(tuple, eff);
        assert(eff.get_predicate_symbol_idx() ==
               new_relation[eff.get_predicate_symbol_idx()].predicate_symbol);
        if (eff.is_negated()) {
            // Remove from relation
            new_relation[eff.get_predicate_symbol_idx()].tuples.erase(ga);
        }
        else {
            int predicate_symbol_idx = eff.get_predicate_symbol_idx();
            if (find(new_relation[predicate_symbol_idx].tuples.begin(),
                     new_relation[predicate_symbol_idx].tuples.end(),
                     ga) == new_relation[predicate_symbol_idx].tuples.end()) {
                // If ground atom is not in the state, we add it
                new_relation[eff.get_predicate_symbol_idx()].tuples.insert(ga);
                add_to_added_atoms(eff.get_predicate_symbol_idx(), ga);
            }
        }
    }
}

DBState CliqueSuccessorGenerator::generate_successor(const LiftedOperatorId &op,
                                                     const ActionSchema &action,
                                                     const DBState &state) {
    added_atoms.clear();
    vector<bool> new_nullary_atoms(state.get_nullary_atoms());
    vector<Relation> new_relation(state.get_relations());
    apply_nullary_effects(action, new_nullary_atoms);

    if (action.is_ground()) {
        apply_ground_action_effects(action, new_relation);
    }
    else {
        apply_lifted_action_effects(action, op.get_instantiation(), new_relation);
    }

    return DBState(std::move(new_relation), std::move(new_nullary_atoms));
}

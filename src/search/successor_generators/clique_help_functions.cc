#include "clique_help_functions.h"

using namespace std;

bool literal_holds(const LiftedOperatorId &op,
                   const vector<Atom> &literals,
                   const DBState &state,
                   const size_t min_test_arity) {
    const auto &operator_instantiation = op.get_instantiation();

    for (const auto &literal : literals) {
        if (literal.get_arguments().size() >= min_test_arity) {
            GroundAtom relation_instance;
            for (const auto argument : literal.get_arguments()) {
                if (argument.is_constant()) {
                    relation_instance.push_back(argument.get_index());
                }
                else {
                    relation_instance.push_back(operator_instantiation.at(argument.get_index()));
                }
            }

            const auto &relation_tuples =
                state.get_tuples_of_relation(literal.get_predicate_symbol_idx());
            const auto relation_exists =
                relation_tuples.find(relation_instance) != relation_tuples.end();

            if (relation_exists == literal.is_negated()) {
                return false;
            }
        }
    }

    return true;
}

bool literal_holds(const LiftedOperatorId &op,
                   const vector<Atom> &dynamic_literals,
                   const vector<Atom> &static_literals,
                   const DBState &dynamic_state,
                   const DBState &static_state,
                   const size_t min_test_arity) {
    return literal_holds(op, static_literals, static_state, min_test_arity) &&
           literal_holds(op, dynamic_literals, dynamic_state, min_test_arity);
}

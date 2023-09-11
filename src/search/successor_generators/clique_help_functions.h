#include "../action.h"
#include "../atom.h"
#include "../states/state.h"

#include <vector>

inline size_t get_rank(int first_position,
                       int first_object,
                       int second_position,
                       int second_object,
                       int arity,
                       int num_objects) {
    const auto first = 1;
    const auto second = first * (arity + 1);
    const auto third = second * (arity + 1);
    const auto fourth = third * (num_objects + 1);
    const auto rank = (first * (first_position + 1)) + (second * (second_position + 1)) +
                      (third * (first_object + 1)) + (fourth * (second_object + 1));
    return (size_t)rank;
}

inline size_t total_ranks(int arity, int num_objects) {
    const auto first = 1;
    const auto second = first * (arity + 1);
    const auto third = second * (arity + 1);
    const auto fourth = third * (num_objects + 1);
    const auto max =
        (first * arity) + (second * arity) + (third * num_objects) + (fourth * num_objects);
    return (size_t)(max + 1);
}

bool literal_holds(const LiftedOperatorId &op,
                   const std::vector<Atom> &literals,
                   const DBState &state,
                   const size_t min_test_arity);

bool literal_holds(const LiftedOperatorId &op,
                   const std::vector<Atom> &dynamic_literals,
                   const std::vector<Atom> &static_literals,
                   const DBState &dynamic_state,
                   const DBState &static_state,
                   const size_t min_test_arity);

#include "dual_queue_bfws.h"

#include "search.h"
#include "utils.h"

#include "../parallel_hashmap/phmap.h"

#include "../successor_generators/successor_generator.h"

#include "../states/extensional_states.h"
#include "../states/sparse_states.h"

#include <algorithm>
#include <iostream>
#include <vector>

using namespace std;

template<class PackedStateT>
utils::ExitCode DualQueueBFWS<PackedStateT>::search(const Task &task,
                                                    SuccessorGenerator &generator,
                                                    Heuristic &heuristic) {
    cout << "Starting Dual-Queue BFWS" << endl;
    clock_t timer_start = clock();
    const auto action_schemas = task.get_action_schemas();
    StatePackerT packer(task);

    Goalcount gc;

    size_t number_goal_conditions = task.get_goal().goal.size() + task.get_goal().positive_nullary_goals.size() + task.get_goal().negative_nullary_goals.size();
    size_t number_relevant_atoms;

    FFHeuristic delete_free_h(task);

    atom_counter = initialize_counter_with_useful_atoms(task, delete_free_h);
    number_relevant_atoms = atom_counter.get_total_number_of_atoms();

    // We use a GreedyOpenList (ordered by the novelty value) for now. This is done to make the
    // search algorithm complete.
    TieBreakingOpenList regular_open_list;
    TieBreakingOpenList preferred_open_list;

    phmap::flat_hash_map<int, NodeNovelty> map_state_to_evaluators;

    SearchNode& root_node = space.insert_or_get_previous_node(packer.pack(task.initial_state),
                                                              LiftedOperatorId::no_operator, StateID::no_state);
    utils::Timer t;

    StandardNovelty novelty_evaluator(task, number_goal_conditions, number_relevant_atoms, width);

    int gc_h0 = gc.compute_heuristic(task.initial_state, task);

    int unachieved_atoms_s0 = 0;
    int novelty_value = novelty_evaluator.compute_novelty(task, task.initial_state, gc_h0, unachieved_atoms_s0);

    root_node.open(0, novelty_value);

    statistics.inc_evaluations();
    cout << "Initial heuristic value " << heuristic_layer << endl;
    statistics.report_f_value_progress(heuristic_layer);
    regular_open_list.do_insertion(root_node.state_id, {novelty_value,
                                                        gc_h0,
                                                        0});

    map_state_to_evaluators.insert({root_node.state_id.id(), NodeNovelty(gc_h0, unachieved_atoms_s0)});

    if (check_goal(task, generator, timer_start, task.initial_state, root_node, space)) return utils::ExitCode::SUCCESS;

    int goalcount_layer = gc_h0;
    while ((not regular_open_list.empty()) or (not preferred_open_list.empty())) {
        StateID sid = get_top_node(preferred_open_list, regular_open_list);
        SearchNode &node = space.get_node(sid);
        int g = node.g;
        if (node.status == SearchNode::Status::CLOSED) {
            continue;
        }
        node.close();
        statistics.report_f_value_progress(g); // In GBFS f = h.
        statistics.inc_expanded();

        assert(sid.id() >= 0 && (unsigned) sid.id() < space.size());

        DBState state = packer.unpack(space.get_state(sid));

        int unsatisfied_goal_parent = map_state_to_evaluators.at(sid.id()).unsatisfied_goals;
        int unsatisfied_relevant_atoms_parent = map_state_to_evaluators.at(sid.id()).unsatisfied_relevant_atoms;

        if (unsatisfied_goal_parent < goalcount_layer) {
            goalcount_layer = unsatisfied_goal_parent;
            boost_priority_queue();
        }

        const auto applicable = generator.get_applicable_actions(action_schemas, state);
        statistics.inc_generated(applicable.size());

        for (const LiftedOperatorId& op_id:applicable) {
            const auto &action = action_schemas[op_id.get_index()];
            DBState s = generator.generate_successor(op_id, action, state);
            auto& child_node = space.insert_or_get_previous_node(packer.pack(s), op_id, node.state_id);
            if (child_node.status != SearchNode::Status::NEW)
                continue;
            bool is_preferred = is_useful_operator(task, s, delete_free_h.get_useful_atoms());
            int dist = g + action.get_cost();
            int unsatisfied_goals = gc.compute_heuristic(s, task);
            int unsatisfied_relevant_atoms = 0;

            unsatisfied_relevant_atoms = atom_counter.count_unachieved_atoms(s, task);

            if (only_effects_opt and (unsatisfied_goals == unsatisfied_goal_parent) and (unsatisfied_relevant_atoms == unsatisfied_relevant_atoms_parent)) {
                novelty_value = novelty_evaluator.compute_novelty_from_operator(task,
                                                                                s,
                                                                                unsatisfied_goals,
                                                                                unsatisfied_relevant_atoms,
                                                                                generator.get_added_atoms());
            }
            else {
                novelty_value = novelty_evaluator.compute_novelty(task,
                                                                  s,
                                                                  unsatisfied_goals,
                                                                  unsatisfied_relevant_atoms);

            }

            statistics.inc_evaluations();
            statistics.inc_evaluated_states();

            child_node.open(dist, novelty_value);
            if (check_goal(task, generator, timer_start, s, child_node, space)) return utils::ExitCode::SUCCESS;
            if (is_preferred) {
                preferred_open_list.do_insertion(child_node.state_id,
                                                    {novelty_value, unsatisfied_goals, dist});
            }
            // Using a boosted dual queue, it is needed to insert every node in the regular
            // open list for completeness.
            regular_open_list.do_insertion(child_node.state_id,{novelty_value, unsatisfied_goals, dist});
            map_state_to_evaluators.insert({child_node.state_id.id(), NodeNovelty(unsatisfied_goals, unsatisfied_relevant_atoms)});
        }
    }

    print_no_solution_found(timer_start);

    return utils::ExitCode::SEARCH_UNSOLVABLE;

}

template <class PackedStateT>
void DualQueueBFWS<PackedStateT>::print_statistics() const {
    statistics.print_detailed_statistics();
    space.print_statistics();
}


template<class PackedStateT>
AtomCounter DualQueueBFWS<PackedStateT>::initialize_counter_with_gc(const Task &task) {
    std::vector<std::vector<GroundAtom>> atoms(task.initial_state.get_relations().size(), std::vector<GroundAtom>());
    std::unordered_set<int> positive = task.get_goal().positive_nullary_goals;
    std::unordered_set<int> negative = task.get_goal().negative_nullary_goals;

    for (const AtomicGoal &atomic_goal : task.get_goal().goal) {
        size_t pred_idx = atomic_goal.get_predicate_index();
        if (task.predicates[pred_idx].isStaticPredicate())
            continue;
        atoms[pred_idx].push_back(atomic_goal.get_arguments());
    }

    return AtomCounter(atoms, positive, negative);
}


template<class PackedStateT>
AtomCounter DualQueueBFWS<PackedStateT>::initialize_counter_with_useful_atoms(const Task &task,
                                                                              FFHeuristic &delete_free_h) const {
    std::vector<std::vector<GroundAtom>> atoms(task.initial_state.get_relations().size(), std::vector<GroundAtom>());
    std::unordered_set<int> positive = task.get_goal().positive_nullary_goals;
    std::unordered_set<int> negative = task.get_goal().negative_nullary_goals;

    int h = delete_free_h.compute_heuristic(task.initial_state, task);
    std::cout << "Initial h-add value of the task: " << h << std::endl;

    std::vector<bool> useful_nullary = delete_free_h.get_useful_nullary_atoms();
    for (size_t i = 0; i < useful_nullary.size(); ++i) {
        if (useful_nullary[i]) {
            positive.insert(i);
        }
    }

    const std::vector<std::vector<GroundAtom>> &useful_atoms = delete_free_h.get_useful_atoms();
    int pred_idx = 0;
    for (const auto &entry : useful_atoms) {
        if (task.predicates[pred_idx].isStaticPredicate()) {
            ++pred_idx;
            continue;
        }
        for (const GroundAtom &atom : entry) {
            atoms[pred_idx].push_back(atom);
        }
        ++pred_idx;
    }

    return AtomCounter(atoms, positive, negative);
}

template<class PackedStateT>
void DualQueueBFWS<PackedStateT>::boost_priority_queue() {
    priority_preferred += BOOST_PREF_OPEN_LIST;
}

// explicit template instantiations
template class DualQueueBFWS<SparsePackedState>;
template class DualQueueBFWS<ExtensionalPackedState>;//

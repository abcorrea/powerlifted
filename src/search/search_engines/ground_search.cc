#include "ground_search.h"

#include "utils.h"

#include "../action.h"
#include "../task.h"

#include "../grounding/ground_task.h"
#include "../heuristics/ground_delete_relaxation_heuristic.h"
#include "../heuristics/ground_landmarks.h"
#include "../states/ground_state.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <unordered_map>

using namespace std;
using namespace ground_state;

namespace {
// Priority of the preferred open lists after every heuristic improvement.
const int PREFERRED_PRIORITY_BOOST = 1000;

// Give up computing landmarks after this many set operations.
const long long MAX_LANDMARK_WORK = 200000000;

// Forced chains and relaxed plan lookahead stop after this many operators.
const int MAX_CHAIN_LENGTH = 10000;

// Operator reference of forced chain k (see GroundSearch::parent_op).
int get_chain_ref(int k) { return -k - 2; }

int get_chain_index(int op_ref) { return -op_ref - 2; }

void print_goal_found_time(clock_t timer_start)
{
    double time = double(clock() - timer_start) / CLOCKS_PER_SEC;
    cout << "Goal found at: " << time << endl;
    cout << "Total time: " << time << endl;
}

}  // namespace

GroundSearch::GroundSearch(const Task &task,
                           shared_ptr<GroundTask> ground_task_,
                           GroundDeleteRelaxationHeuristic::Type heuristic_type,
                           int width,
                           bool use_landmarks,
                           bool full_novelty_check)
    : task(task),
      ground_task(std::move(ground_task_)),
      use_landmarks(use_landmarks),
      full_novelty_check(full_novelty_check),
      generator(*ground_task),
      heuristic(make_unique<GroundDeleteRelaxationHeuristic>(task, ground_task, heuristic_type)),
      num_words(get_num_words(ground_task->num_facts())),
      num_base_facts(heuristic->get_num_base_facts()),
      goal_facts(ground_task->goal),
      negated_goal_facts(ground_task->negated_goal),
      registry(ground_task->num_facts()),
      novelty(width, ground_task->num_facts())
{
    generator.restrict_operators(heuristic->get_relevant_actions());
    if (use_landmarks)
        lazy_novelty = make_unique<GroundNovelty>(width, ground_task->num_facts());
}

GroundSearch::~GroundSearch() = default;

int GroundSearch::count_unsatisfied_goals(const uint64_t *bits) const
{
    int count = 0;
    for (int f : goal_facts) {
        if (!test_bit(bits, f))
            ++count;
    }
    for (int f : negated_goal_facts) {
        if (test_bit(bits, f))
            ++count;
    }
    return count;
}

int GroundSearch::count_unsatisfied_relevant(const uint64_t *bits) const
{
    int count = 0;
    for (int f : relevant_facts) {
        if (!test_bit(bits, f))
            ++count;
    }
    for (int f : relevant_negated_facts) {
        if (test_bit(bits, f))
            ++count;
    }
    return count;
}

long long GroundSearch::get_novelty_partition(int unsatisfied_goals,
                                              int unsatisfied_relevant,
                                              int h_landmarks) const
{
    if (landmarks)
        return h_landmarks;
    long long num_goal_values = goal_facts.size() + negated_goal_facts.size() + 1;
    return unsatisfied_relevant * num_goal_values + unsatisfied_goals;
}

uint64_t *GroundSearch::get_accepted_landmarks(int state_id)
{
    return accepted_landmarks.data() + size_t(state_id) * num_landmark_words;
}

void GroundSearch::add_node(int state_id)
{
    if (state_id < int(parent.size()))
        return;
    parent.resize(state_id + 1, -1);
    parent_op.resize(state_id + 1, -1);
    g_value.resize(state_id + 1, 0);
    num_unsatisfied_goals.resize(state_id + 1, 0);
    num_unsatisfied_relevant.resize(state_id + 1, 0);
    closed.resize(state_id + 1, 0);
    if (num_landmark_words)
        accepted_landmarks.resize(size_t(state_id + 1) * num_landmark_words, 0);
}

int GroundSearch::compute_novelty(const uint64_t *bits, long long partition)
{
    if (novelty.get_width() > 1)
        get_true_facts(bits, num_words, child_facts);
    return novelty.compute(bits, child_facts, partition);
}

void GroundSearch::insert(int state_id,
                          int h,
                          int h_landmarks,
                          int g,
                          int unsatisfied_goals,
                          int novelty_value,
                          bool preferred)
{
    for (size_t i = 0; i < open_list_types.size(); ++i) {
        array<int, 3> key{};
        switch (open_list_types[i]) {
        case NOVELTY:
            key = {novelty_value, landmarks ? h_landmarks : unsatisfied_goals, g};
            break;
        case LAZY_NOVELTY:
            key = {parent_lazy_novelty, landmarks ? parent_h_landmarks : unsatisfied_goals, g};
            break;
        case HEURISTIC:
            key = {h, g, 0};
            break;
        case LANDMARKS:
            key = {h_landmarks, h, 0};
            break;
        }
        regular_open_lists[i].push(key, state_id);
        if (preferred)
            preferred_open_lists[i].push(key, state_id);
    }
}

int GroundSearch::pop()
{
    size_t num_open_lists = open_list_types.size();
    for (size_t k = 0; k < num_open_lists; ++k) {
        size_t i = (next_open_list + k) % num_open_lists;
        KeyedOpenList<3> &regular = regular_open_lists[i];
        KeyedOpenList<3> &preferred = preferred_open_lists[i];
        if (regular.empty() && preferred.empty())
            continue;
        next_open_list = i + 1;
        if ((preferred_priority > 0 && !preferred.empty()) || regular.empty()) {
            --preferred_priority;
            return preferred.pop();
        }
        return regular.pop();
    }
    return -1;
}

/*
  Register the initial state and insert it into the open lists, which are
  set up here because the landmarks are only known now.
*/
void GroundSearch::initialize_search(const vector<uint64_t> &initial_bits)
{
    open_list_types.push_back(NOVELTY);
    if (lazy_novelty)
        open_list_types.push_back(LAZY_NOVELTY);
    open_list_types.push_back(HEURISTIC);
    if (landmarks)
        open_list_types.push_back(LANDMARKS);
    regular_open_lists.resize(open_list_types.size());
    preferred_open_lists.resize(open_list_types.size());
    preferred_priority = PREFERRED_PRIORITY_BOOST;

    if (landmarks)
        num_landmark_words = landmarks->get_num_words();
    child_bits.assign(num_words, 0);
    chain_bits.assign(num_words, 0);
    parent_accepted.assign(max(num_landmark_words, 1), 0);
    child_accepted.assign(max(num_landmark_words, 1), 0);
    chain_accepted.assign(max(num_landmark_words, 1), 0);
    fact_marks.assign(ground_task->num_facts(), 0);

    if (registry.is_compact())
        cout << "Storing states compactly (" << ground_task->num_facts() << " facts)." << endl;
    state_bits = initial_bits;
    int root = registry.insert(state_bits.data()).first;
    add_node(root);
    const uint64_t *root_bits = state_bits.data();
    num_unsatisfied_goals[root] = count_unsatisfied_goals(root_bits);
    num_unsatisfied_relevant[root] = count_unsatisfied_relevant(root_bits);
    int h_landmarks = 0;
    if (landmarks) {
        landmarks->initial_accepted(root_bits, get_accepted_landmarks(root));
        h_landmarks = landmarks->compute(get_accepted_landmarks(root), root_bits);
    }
    compute_novelty(root_bits,
                    get_novelty_partition(
                        num_unsatisfied_goals[root], num_unsatisfied_relevant[root], h_landmarks));
    best_h_landmarks = h_landmarks;
    insert(root, best_h, h_landmarks, 0, num_unsatisfied_goals[root], 1, false);

    cout << "Open lists: novelty (width " << novelty.get_width() << ")"
         << (lazy_novelty ? ", lazy novelty" : "") << ", heuristic"
         << (landmarks ? ", landmarks" : "") << endl;
}

void GroundSearch::expand(int state_id)
{
    // Copy: the registry may grow while generating successors.
    registry.copy_to(state_id, state_bits.data());
    get_true_facts(state_bits.data(), num_words, state_facts);
    int h = heuristic->compute_for_facts(state_facts, false);
    statistics.inc_evaluations();
    statistics.inc_evaluated_states();
    if (h == UNSOLVABLE_STATE) {
        statistics.inc_dead_ends();
        return;
    }
    bool progress = false;
    if (h < best_h) {
        best_h = h;
        progress = true;
    }
    if (landmarks) {
        memcpy(parent_accepted.data(),
               get_accepted_landmarks(state_id),
               sizeof(uint64_t) * num_landmark_words);
        parent_h_landmarks = landmarks->compute(parent_accepted.data(), state_bits.data());
        if (parent_h_landmarks < best_h_landmarks) {
            best_h_landmarks = parent_h_landmarks;
            progress = true;
        }
    }
    if (lazy_novelty) {
        if (lazy_novelty->get_width() > 1)
            get_true_facts(state_bits.data(), num_words, child_facts);
        parent_lazy_novelty =
            lazy_novelty->compute(state_bits.data(),
                                  child_facts,
                                  get_novelty_partition(num_unsatisfied_goals[state_id],
                                                        num_unsatisfied_relevant[state_id],
                                                        parent_h_landmarks));
    }
    if (progress) {
        preferred_priority += PREFERRED_PRIORITY_BOOST;
        cout << "New heuristic value expanded: h=" << best_h;
        if (landmarks)
            cout << ", h_landmarks=" << best_h_landmarks;
        cout << " [expansions: " << statistics.get_expanded()
             << ", evaluations: " << statistics.get_evaluations()
             << ", generations: " << statistics.get_generated() << "]" << endl;
    }

    // Mark the atoms of the relaxed plan and the interesting landmarks.
    auto mark = [&](int fact, FactMark base_mark, FactMark complement_mark) {
        int base_fact = fact;
        FactMark fact_mark = base_mark;
        if (fact >= num_base_facts) {
            base_fact = heuristic->get_complement_base(fact);
            fact_mark = complement_mark;
        }
        fact_marks[base_fact] |= fact_mark;
        marked_facts.push_back(base_fact);
    };
    marked_facts.clear();
    for (int f : heuristic->get_useful_facts())
        mark(f, IN_RELAXED_PLAN, COMPLEMENT_IN_RELAXED_PLAN);
    if (landmarks) {
        landmarks->get_interesting_facts(
            parent_accepted.data(), state_bits.data(), interesting_facts);
        for (int f : interesting_facts)
            mark(f, INTERESTING_LANDMARK, COMPLEMENT_INTERESTING_LANDMARK);
    }

    generator.get_applicable(state_bits.data(), applicable);
    statistics.inc_generated(applicable.size());
    for (int op : applicable) {
        added.clear();
        removed.clear();
        generator.apply(state_bits.data(), op, child_bits.data(), added, removed);
        int g = g_value[state_id] + ground_task->operators[op].cost;
        int op_ref = op;
        if (landmarks)
            landmarks->update_accepted(
                parent_accepted.data(), child_bits.data(), child_accepted.data());
        chain.assign(1, op);
        if (!follow_forced_chain(g, true)) {
            statistics.inc_pruned_states();
            continue;
        }
        if (chain.size() > 1) {
            ++num_forced_chains;
            forced_chain_length_sum += chain.size();
            added.clear();
            removed.clear();
            get_changes(state_bits.data(), child_bits.data(), num_words, added, removed);
            op_ref = get_chain_ref(chains.size());
        }
        if (!insert_child(state_id, op_ref, g, h, false))
            return;
    }
    if (h > 0)
        relaxed_plan_lookahead(state_id, h);
    for (int f : marked_facts)
        fact_marks[f] = 0;
}

/*
  Apply forced operators to the state in child_bits while it is not a goal
  and has exactly one applicable operator, appending them to `chain` and
  adding their cost to `g` (and updating child_accepted if
  `update_landmarks` is set). Returns false if the chain ends in a dead end:
  a state without applicable operators, or a cycle (detected with Brent's
  algorithm), which never reaches a goal.
*/
bool GroundSearch::follow_forced_chain(int &g, bool update_landmarks)
{
    cycle_bits = child_bits;
    int power = 1;
    int length = 0;
    for (int step = 0; step < MAX_CHAIN_LENGTH; ++step) {
        if (count_unsatisfied_goals(child_bits.data()) == 0)
            break;
        // Only whether there are 0, 1, or more applicable operators matters.
        generator.get_applicable(child_bits.data(), chain_ops, 2);
        if (chain_ops.empty())
            return false;
        if (chain_ops.size() != 1)
            break;
        int op = chain_ops[0];
        chain_added.clear();
        chain_removed.clear();
        generator.apply(child_bits.data(), op, chain_bits.data(), chain_added, chain_removed);
        child_bits.swap(chain_bits);
        chain.push_back(op);
        g += ground_task->operators[op].cost;
        if (update_landmarks && landmarks) {
            landmarks->update_accepted(
                child_accepted.data(), child_bits.data(), chain_accepted.data());
            child_accepted.swap(chain_accepted);
        }
        if (child_bits == cycle_bits)
            return false;
        if (++length == power) {
            cycle_bits = child_bits;
            power *= 2;
            length = 0;
        }
    }
    return true;
}

/*
  Register the successor in child_bits, reached from `state_id` by `op_ref`
  with cost `g` (added, removed, and child_accepted describe it), and insert
  it into the open lists. Returns false if it is a goal state.
*/
bool GroundSearch::insert_child(int state_id, int op_ref, int g, int h, bool force_preferred)
{
    auto [child_id, is_new] = registry.insert(child_bits.data());
    if (op_ref < -1 && (is_new || g < g_value[child_id]))
        chains.push_back(chain);
    if (!is_new) {
        if (g < g_value[child_id]) {
            g_value[child_id] = g;
            parent[child_id] = state_id;
            parent_op[child_id] = op_ref;
            if (closed[child_id]) {
                closed[child_id] = 0;
                statistics.inc_reopened();
                int h_landmarks = landmarks ? landmarks->compute(get_accepted_landmarks(child_id),
                                                                 child_bits.data())
                                            : 0;
                int novelty_value =
                    compute_novelty(child_bits.data(),
                                    get_novelty_partition(num_unsatisfied_goals[child_id],
                                                          num_unsatisfied_relevant[child_id],
                                                          h_landmarks));
                insert(child_id,
                       h,
                       h_landmarks,
                       g,
                       num_unsatisfied_goals[child_id],
                       novelty_value,
                       false);
            }
        }
        return true;
    }
    add_node(child_id);
    parent[child_id] = state_id;
    parent_op[child_id] = op_ref;
    g_value[child_id] = g;
    int unsatisfied_goals = count_unsatisfied_goals(child_bits.data());
    int unsatisfied_relevant = count_unsatisfied_relevant(child_bits.data());
    num_unsatisfied_goals[child_id] = unsatisfied_goals;
    num_unsatisfied_relevant[child_id] = unsatisfied_relevant;
    if (unsatisfied_goals == 0) {
        goal_state = child_id;
        return false;
    }
    int h_landmarks = 0;
    if (landmarks) {
        memcpy(get_accepted_landmarks(child_id),
               child_accepted.data(),
               sizeof(uint64_t) * num_landmark_words);
        h_landmarks = landmarks->compute(get_accepted_landmarks(child_id), child_bits.data());
    }
    long long partition =
        get_novelty_partition(unsatisfied_goals, unsatisfied_relevant, h_landmarks);
    long long parent_partition = get_novelty_partition(
        num_unsatisfied_goals[state_id], num_unsatisfied_relevant[state_id], parent_h_landmarks);
    int novelty_value;
    if (!full_novelty_check && partition == parent_partition) {
        if (novelty.get_width() > 1)
            get_true_facts(child_bits.data(), num_words, child_facts);
        novelty_value = novelty.compute_for_added_facts(added, child_facts, partition);
    }
    else {
        novelty_value = compute_novelty(child_bits.data(), partition);
    }
    bool preferred = force_preferred;
    for (int f : added) {
        if (fact_marks[f] & (IN_RELAXED_PLAN | INTERESTING_LANDMARK)) {
            preferred = true;
            break;
        }
    }
    if (!preferred) {
        for (int f : removed) {
            if (fact_marks[f] & (COMPLEMENT_IN_RELAXED_PLAN | COMPLEMENT_INTERESTING_LANDMARK)) {
                preferred = true;
                break;
            }
        }
    }
    insert(child_id, h, h_landmarks, g, unsatisfied_goals, novelty_value, preferred);
    return true;
}

/*
  Relaxed plan lookahead (Vidal 2004): apply the operators of the relaxed
  plan of the expanded state, in the order of their h-add layers, as long as
  some of them is applicable; the resulting state is added as a preferred
  successor.
*/
void GroundSearch::relaxed_plan_lookahead(int state_id, int h)
{
    const vector<int> &relaxed_plan = heuristic->get_relaxed_plan();
    const vector<int> &levels = heuristic->get_relaxed_plan_levels();
    if (relaxed_plan.size() < 2)
        return;
    lookahead_order.resize(relaxed_plan.size());
    for (size_t i = 0; i < relaxed_plan.size(); ++i)
        lookahead_order[i] = i;
    stable_sort(lookahead_order.begin(), lookahead_order.end(), [&](int a, int b) {
        return levels[a] < levels[b];
    });
    vector<char> used(relaxed_plan.size(), 0);
    child_bits = state_bits;
    chain.clear();
    int g = g_value[state_id];
    while (true) {
        int found = -1;
        for (int i : lookahead_order) {
            if (!used[i] && generator.is_applicable(child_bits.data(), relaxed_plan[i])) {
                found = i;
                break;
            }
        }
        if (found < 0)
            break;
        used[found] = 1;
        int op = relaxed_plan[found];
        chain_added.clear();
        chain_removed.clear();
        generator.apply(child_bits.data(), op, chain_bits.data(), chain_added, chain_removed);
        child_bits.swap(chain_bits);
        chain.push_back(op);
        g += ground_task->operators[op].cost;
        if (count_unsatisfied_goals(child_bits.data()) == 0)
            break;
    }
    if (chain.size() < 2)
        return;
    // Continue with forced operators (e.g., to finish a sequence of
    // operators that only makes sense as a whole).
    if (!follow_forced_chain(g, false))
        return;
    ++num_lookahead_states;
    added.clear();
    removed.clear();
    get_changes(state_bits.data(), child_bits.data(), num_words, added, removed);
    if (landmarks)
        landmarks->update_accepted(
            parent_accepted.data(), child_bits.data(), child_accepted.data());
    insert_child(state_id, get_chain_ref(chains.size()), g, h, true);
}

void GroundSearch::extract_plan(vector<int> &plan) const
{
    for (int s = goal_state; parent[s] >= 0; s = parent[s]) {
        int op_ref = parent_op[s];
        if (op_ref >= 0) {
            plan.push_back(op_ref);
        }
        else {
            const vector<int> &ops = chains[get_chain_index(op_ref)];
            plan.insert(plan.end(), ops.rbegin(), ops.rend());
        }
    }
    reverse(plan.begin(), plan.end());
}

void GroundSearch::print_ground_plan(const vector<int> &plan) const
{
    vector<LiftedOperatorId> lifted_plan;
    for (int op : plan) {
        const GroundOperator &ground_op = ground_task->operators[op];
        vector<int> instantiation = ground_op.instantiation;
        lifted_plan.emplace_back(
            ground_op.schema, std::move(instantiation), unordered_map<int, int>());
    }
    print_plan(lifted_plan, task);
}

utils::ExitCode GroundSearch::search()
{
    cout << "Starting ground search" << endl;
    clock_t timer_start = clock();

    if (ground_task->goal_unreachable) {
        cout << "Goal is not relaxed reachable." << endl;
        print_no_solution_found(timer_start);
        return utils::ExitCode::SEARCH_UNSOLVABLE;
    }

    vector<uint64_t> initial_bits(num_words, 0);
    for (int f : ground_task->initial_facts)
        set_bit(initial_bits.data(), f);
    generator.evaluate_axioms(initial_bits.data());
    get_true_facts(initial_bits.data(), num_words, state_facts);
    best_h = heuristic->compute_for_facts(state_facts, false);
    cout << "Initial heuristic value " << best_h << endl;
    vector<int> relevant = heuristic->get_useful_facts();
    sort(relevant.begin(), relevant.end());
    cout << "Total number of relevant atoms: " << relevant.size() << endl;
    for (int f : relevant) {
        if (f < num_base_facts)
            relevant_facts.push_back(f);
        else
            relevant_negated_facts.push_back(heuristic->get_complement_base(f));
    }

    if (count_unsatisfied_goals(initial_bits.data()) == 0) {
        print_goal_found_time(timer_start);
        print_ground_plan({});
        return utils::ExitCode::SUCCESS;
    }
    if (best_h == UNSOLVABLE_STATE) {
        print_no_solution_found(timer_start);
        return utils::ExitCode::SEARCH_UNSOLVABLE;
    }

    if (use_landmarks) {
        vector<int> initial_relaxed_facts = state_facts;
        heuristic->add_complement_facts(state_facts, initial_relaxed_facts);
        landmarks =
            make_unique<GroundLandmarks>(*heuristic, initial_relaxed_facts, MAX_LANDMARK_WORK);
        if (landmarks->is_computed())
            cout << "Initial landmark count: " << landmarks->get_num_landmarks() << endl;
        else
            landmarks.reset();
    }

    initialize_search(initial_bits);
    while (true) {
        int state_id = pop();
        if (state_id < 0)
            break;
        if (closed[state_id])
            continue;
        closed[state_id] = 1;
        statistics.inc_expanded();
        expand(state_id);
        int expanded = statistics.get_expanded();
        if (expanded == 32 || expanded == 256) {
            for (GroundNovelty *tables : {&novelty, lazy_novelty.get()}) {
                if (tables && tables->is_too_expensive())
                    tables->downgrade_to_width_1();
            }
        }
        if (goal_state >= 0) {
            print_goal_found_time(timer_start);
            vector<int> plan;
            extract_plan(plan);
            print_ground_plan(plan);
            return utils::ExitCode::SUCCESS;
        }
    }
    // Complete search: only dead ends are pruned.
    print_no_solution_found(timer_start);
    return utils::ExitCode::SEARCH_UNSOLVABLE;
}

void GroundSearch::print_statistics() const
{
    statistics.print_detailed_statistics();
    cout << "Registered " << registry.size() << " state(s)." << endl;
    cout << "Forced chains: " << num_forced_chains << " (average length "
         << (num_forced_chains ? double(forced_chain_length_sum) / num_forced_chains : 0.0) << ")"
         << endl;
    cout << "Relaxed plan lookahead states: " << num_lookahead_states << endl;
}

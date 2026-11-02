#ifndef SEARCH_SEARCH_ENGINES_GROUND_SEARCH_H
#define SEARCH_SEARCH_ENGINES_GROUND_SEARCH_H

#include "../search_statistics.h"

#include "../heuristics/ground_delete_relaxation_heuristic.h"
#include "../novelty/ground_novelty.h"
#include "../open_lists/keyed_open_list.h"
#include "../states/ground_state_registry.h"
#include "../successor_generators/ground_successor_generator.h"
#include "../utils/system.h"

#include <cstdint>
#include <ctime>
#include <memory>
#include <string>
#include <vector>

class GroundLandmarks;
class GroundTask;
class Task;

/**
 * @brief Greedy best-first width search on the ground task (see GroundTask).
 *
 * The search alternates (round robin) between the following open lists,
 * each with a preferred variant (successors achieving an atom of the
 * parent's relaxed plan or an interesting landmark) that gets priority after
 * every heuristic improvement:
 *  - novelty (width 1 or 2), ties broken by g;
 *  - with landmarks only: lazy novelty (the novelty of the parent in
 *    separate novelty tables), ties broken by g;
 *  - the heuristic value of the parent (lazy evaluation), ties broken by g;
 *  - with landmarks only: the landmark-count heuristic of the state, ties
 *    broken by the heuristic value of the parent.
 * Without landmarks, novelty is partitioned by the number of unsatisfied
 * goals and of unsatisfied relevant atoms (the atoms of the relaxed plan of
 * the initial state), as in alternated BFWS (Correa and Seipp 2022); with
 * landmarks, it is partitioned by the landmark count, as in NOLAN (Correa and
 * Seipp 2025).
 *
 * Forced chains (successors with exactly one applicable operator) are
 * followed without registering the intermediate states; chains ending in a
 * state without applicable operators or in a cycle are pruned. After every
 * evaluation, the relaxed plan is applied greedily (relaxed plan lookahead)
 * and the resulting state is added as a preferred successor.
 */
class GroundSearch {
    const Task &task;
    std::shared_ptr<GroundTask> ground_task;
    bool use_landmarks;
    bool full_novelty_check;

    GroundSuccessorGenerator generator;
    std::unique_ptr<GroundDeleteRelaxationHeuristic> heuristic;
    std::unique_ptr<GroundLandmarks> landmarks;
    int num_words;
    int num_base_facts;
    int num_landmark_words = 0;
    std::vector<int> goal_facts;
    std::vector<int> negated_goal_facts;
    // Relevant atoms (atoms of the relaxed plan of the initial state); the
    // second list contains the base facts of relevant complement facts.
    std::vector<int> relevant_facts;
    std::vector<int> relevant_negated_facts;

    GroundStateRegistry registry;
    GroundNovelty novelty;
    //! Novelty tables of the lazy novelty open list (with landmarks only).
    std::unique_ptr<GroundNovelty> lazy_novelty;

    // Search nodes, indexed by state id. The operator from the parent is a
    // ground operator, or -k - 2 for the forced chain chains[k].
    std::vector<int> parent;
    std::vector<int> parent_op;
    std::vector<int> g_value;
    std::vector<short> num_unsatisfied_goals;
    std::vector<int> num_unsatisfied_relevant;
    std::vector<char> closed;
    //! Accepted landmarks of each state (num_landmark_words words each).
    std::vector<uint64_t> accepted_landmarks;
    std::vector<std::vector<int>> chains;
    int goal_state = -1;

    enum OpenListType { NOVELTY, LAZY_NOVELTY, HEURISTIC, LANDMARKS };
    std::vector<OpenListType> open_list_types;
    std::vector<KeyedOpenList<3>> regular_open_lists, preferred_open_lists;
    size_t next_open_list = 0;
    int preferred_priority = 0;
    int best_h;
    int best_h_landmarks;
    //! Values of the state being expanded (inherited by its successors).
    int parent_h_landmarks = 0;
    int parent_lazy_novelty = 1;

    /*
      Marks of the base facts for the state being expanded: whether the fact
      or its complement is an atom of the relaxed plan or an interesting
      landmark.
    */
    enum FactMark {
        IN_RELAXED_PLAN = 1,
        COMPLEMENT_IN_RELAXED_PLAN = 2,
        INTERESTING_LANDMARK = 4,
        COMPLEMENT_INTERESTING_LANDMARK = 8
    };
    std::vector<char> fact_marks;
    std::vector<int> marked_facts;

    // Scratch data.
    std::vector<uint64_t> state_bits, child_bits, chain_bits, cycle_bits;
    std::vector<int> state_facts, child_facts, applicable, added, removed;
    std::vector<int> chain, chain_ops, chain_added, chain_removed, interesting_facts;
    std::vector<uint64_t> parent_accepted, child_accepted, chain_accepted;
    std::vector<int> lookahead_order;

    SearchStatistics statistics;
    long long num_forced_chains = 0;
    long long forced_chain_length_sum = 0;
    long long num_lookahead_states = 0;

    int count_unsatisfied_goals(const uint64_t *bits) const;
    int count_unsatisfied_relevant(const uint64_t *bits) const;
    long long
    get_novelty_partition(int unsatisfied_goals, int unsatisfied_relevant, int h_landmarks) const;
    uint64_t *get_accepted_landmarks(int state_id);
    void add_node(int state_id);
    int compute_novelty(const uint64_t *bits, long long partition);

    void insert(int state_id,
                int h,
                int h_landmarks,
                int g,
                int unsatisfied_goals,
                int novelty_value,
                bool preferred);
    int pop();
    void initialize_search(const std::vector<uint64_t> &initial_bits);
    void expand(int state_id);
    bool follow_forced_chain(int &g, bool update_landmarks);
    bool insert_child(int state_id, int op_ref, int g, int h, bool force_preferred);
    void relaxed_plan_lookahead(int state_id, int h);
    void extract_plan(std::vector<int> &plan) const;
    void print_ground_plan(const std::vector<int> &plan) const;

public:
    /**
     * Search with novelty of the given width, using the delete-relaxation
     * heuristic of the given type and, if `use_landmarks`
     * is set, the landmark-count heuristic. With `full_novelty_check`,
     * novelty considers all facts of a state, not only the facts added by
     * the creating operator.
     */
    GroundSearch(const Task &task,
                 std::shared_ptr<GroundTask> ground_task,
                 GroundDeleteRelaxationHeuristic::Type heuristic_type,
                 int width,
                 bool use_landmarks,
                 bool full_novelty_check);
    ~GroundSearch();

    utils::ExitCode search();
    void print_statistics() const;
};

#endif  // SEARCH_SEARCH_ENGINES_GROUND_SEARCH_H

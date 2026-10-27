#include "grounded_search.h"

#include "../action.h"
#include "../task.h"

#include "../grounding/ground_task.h"
#include "../heuristics/grounded_heuristic.h"
#include "../search_engines/utils.h"

#include "landmarks.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <iostream>
#include <map>
#include <unordered_map>

using namespace std;

namespace {
inline bool test_bit(const uint64_t *b, int f) {
    return (b[f >> 6] >> (f & 63)) & 1ULL;
}

inline void set_bit(uint64_t *b, int f) {
    b[f >> 6] |= 1ULL << (f & 63);
}

inline void reset_bit(uint64_t *b, int f) {
    b[f >> 6] &= ~(1ULL << (f & 63));
}

void collect_true_facts(const uint64_t *b, int num_words, vector<int> &out) {
    out.clear();
    for (int w = 0; w < num_words; ++w) {
        uint64_t word = b[w];
        while (word) {
            int t = __builtin_ctzll(word);
            out.push_back(w * 64 + t);
            word &= word - 1;
        }
    }
}

void build_csr(const vector<const vector<int> *> &lists, vector<int> &start, vector<int> &data) {
    start.clear();
    data.clear();
    for (const vector<int> *l : lists) {
        start.push_back(data.size());
        data.insert(data.end(), l->begin(), l->end());
    }
    start.push_back(data.size());
}

void print_goal_time(clock_t timer_start) {
    double t = double(clock() - timer_start) / CLOCKS_PER_SEC;
    cout << "Goal found at: " << t << endl;
    cout << "Total time: " << t << endl;
}

/*
 * Open list with lexicographically ordered integer keys and FIFO order among
 * entries with the same key.
 */
template<int K>
class KeyedOpenList {
    map<array<int, K>, deque<int>> buckets;
    size_t num_entries = 0;

public:
    void push(const array<int, K> &key, int id) {
        buckets[key].push_back(id);
        ++num_entries;
    }

    int pop() {
        auto it = buckets.begin();
        int id = it->second.front();
        it->second.pop_front();
        if (it->second.empty())
            buckets.erase(it);
        --num_entries;
        return id;
    }

    bool empty() const {
        return num_entries == 0;
    }
};
/*
 * Novelty tables (width 1, or width 2 for small tasks), one per partition
 * (#unsatisfied goals, #unsatisfied relevant atoms). Novelty is 1 if the
 * state makes some fact true for the first time in its partition, 2 if it
 * makes some pair of facts true for the first time, and 3 otherwise.
 */
class GroundNovelty {
    struct Table {
        vector<uint64_t> singles;
        vector<uint64_t> pairs;
    };
    int width;
    int num_facts;
    int num_words;
    int num_goals;
    unordered_map<long long, Table> tables;

    Table &get_table(int gc, int rc) {
        long long key = (long long)rc * (num_goals + 1) + gc;
        auto it = tables.find(key);
        if (it == tables.end()) {
            Table t;
            t.singles.assign(num_words, 0);
            if (width > 1)
                t.pairs.assign((size_t(num_facts) * num_facts + 63) / 64, 0);
            it = tables.emplace(key, std::move(t)).first;
        }
        return it->second;
    }

    static bool test_and_set(vector<uint64_t> &bits, size_t i) {
        uint64_t mask = 1ULL << (i & 63);
        uint64_t &word = bits[i >> 6];
        if (word & mask)
            return false;
        word |= mask;
        return true;
    }

public:
    static const int NOT_NOVEL = 3;

    GroundNovelty(int width, int num_facts, int num_goals)
        : width(width), num_facts(num_facts), num_words((num_facts + 63) / 64),
          num_goals(num_goals) {
        if (num_words == 0)
            num_words = 1;
        if (width > 1 && num_facts > 4096) {
            cout << "Too many facts for width-2 novelty; using width 1." << endl;
            this->width = 1;
        }
    }

    int get_width() const {
        return width;
    }

    //! Novelty of the state with the given bits (and, for width 2, true
    //! facts in increasing order).
    int full(const uint64_t *bits, const vector<int> &facts, int gc, int rc) {
        Table &t = get_table(gc, rc);
        bool novel = false;
        for (int w = 0; w < num_words; ++w) {
            if (bits[w] & ~t.singles[w]) {
                novel = true;
                t.singles[w] |= bits[w];
            }
        }
        if (width == 1)
            return novel ? 1 : NOT_NOVEL;
        bool novel_pair = false;
        for (size_t i = 0; i < facts.size(); ++i) {
            size_t base = size_t(facts[i]) * num_facts;
            for (size_t j = i + 1; j < facts.size(); ++j) {
                if (test_and_set(t.pairs, base + facts[j]))
                    novel_pair = true;
            }
        }
        return novel ? 1 : (novel_pair ? 2 : NOT_NOVEL);
    }

    //! Novelty considering only the facts added with respect to the parent
    //! (which belongs to the same partition).
    int effects(const vector<int> &added, const vector<int> &facts, int gc, int rc) {
        Table &t = get_table(gc, rc);
        bool novel = false;
        for (int f : added) {
            if (test_and_set(t.singles, f))
                novel = true;
        }
        if (width == 1)
            return novel ? 1 : NOT_NOVEL;
        bool novel_pair = false;
        for (int a : added) {
            for (int f : facts) {
                if (f == a)
                    continue;
                size_t i = f < a ? size_t(f) * num_facts + a : size_t(a) * num_facts + f;
                if (test_and_set(t.pairs, i))
                    novel_pair = true;
            }
        }
        return novel ? 1 : (novel_pair ? 2 : NOT_NOVEL);
    }
};
}  // namespace


GroundSuccessorGenerator::GroundSuccessorGenerator(const GroundTask &task)
    : task(task), num_words((task.num_facts() + 63) / 64) {
    if (num_words == 0)
        num_words = 1;
    const auto &ops = task.operators;
    vector<const vector<int> *> pre_lists, neg_lists;
    for (const GroundOperator &op : ops) {
        pre_lists.push_back(&op.pre);
        neg_lists.push_back(&op.negated_pre);
    }
    build_csr(pre_lists, pre_start, pre);
    build_csr(neg_lists, neg_start, neg);

    // Watch every operator by its precondition occurring in the fewest
    // operator preconditions.
    int num_facts = task.num_facts();
    vector<int> occurrences(num_facts, 0);
    for (int f : pre)
        ++occurrences[f];
    vector<vector<int>> watched_by(num_facts);
    for (size_t o = 0; o < ops.size(); ++o) {
        if (ops[o].pre.empty()) {
            no_pre_ops.push_back(o);
            continue;
        }
        int best = ops[o].pre[0];
        for (int f : ops[o].pre) {
            if (occurrences[f] < occurrences[best])
                best = f;
        }
        watched_by[best].push_back(o);
    }
    vector<const vector<int> *> watch_lists;
    for (const auto &l : watched_by)
        watch_lists.push_back(&l);
    build_csr(watch_lists, watch_start, watch);

    // Axioms sorted by stratum.
    has_axioms = !task.axioms.empty();
    derived_mask.assign(num_words, 0);
    vector<int> order(task.axioms.size());
    for (size_t i = 0; i < order.size(); ++i)
        order[i] = i;
    stable_sort(order.begin(), order.end(), [&](int a, int b) {
        return task.axioms[a].stratum < task.axioms[b].stratum;
    });
    vector<const vector<int> *> body_lists, neg_body_lists;
    int current_stratum = -1;
    for (size_t k = 0; k < order.size(); ++k) {
        const GroundAxiom &ax = task.axioms[order[k]];
        if (ax.stratum != current_stratum) {
            current_stratum = ax.stratum;
            stratum_start.push_back(k);
        }
        body_lists.push_back(&ax.body);
        neg_body_lists.push_back(&ax.negated_body);
        ax_head.push_back(ax.head);
        ax_stratum.push_back(ax.stratum);
        set_bit(derived_mask.data(), ax.head);
    }
    stratum_start.push_back(order.size());
    build_csr(body_lists, ax_body_start, ax_body);
    build_csr(neg_body_lists, ax_neg_start, ax_neg);
    vector<vector<int>> body_of_lists(num_facts);
    for (size_t k = 0; k < order.size(); ++k) {
        for (int i = ax_body_start[k]; i < ax_body_start[k + 1]; ++i)
            body_of_lists[ax_body[i]].push_back(k);
    }
    vector<const vector<int> *> bol;
    for (const auto &l : body_of_lists)
        bol.push_back(&l);
    build_csr(bol, body_of_start, body_of);
    ax_unsatisfied.assign(order.size(), 0);
}

void GroundSuccessorGenerator::get_applicable(const uint64_t *bits,
                                              const vector<int> &true_facts,
                                              vector<int> &ops) const {
    ops.clear();
    auto check = [&](int o) {
        for (int i = pre_start[o]; i < pre_start[o + 1]; ++i) {
            if (!test_bit(bits, pre[i]))
                return false;
        }
        for (int i = neg_start[o]; i < neg_start[o + 1]; ++i) {
            if (test_bit(bits, neg[i]))
                return false;
        }
        return true;
    };
    for (int o : no_pre_ops) {
        if (check(o))
            ops.push_back(o);
    }
    for (int f : true_facts) {
        for (int i = watch_start[f]; i < watch_start[f + 1]; ++i) {
            int o = watch[i];
            if (check(o))
                ops.push_back(o);
        }
    }
}

void GroundSuccessorGenerator::evaluate_axioms(uint64_t *bits) {
    if (!has_axioms)
        return;
    for (int w = 0; w < num_words; ++w)
        bits[w] &= ~derived_mask[w];
    for (size_t s = 0; s + 1 < stratum_start.size(); ++s) {
        int begin = stratum_start[s], end = stratum_start[s + 1];
        int stratum = ax_stratum[begin];
        ax_queue.clear();
        for (int k = begin; k < end; ++k) {
            bool blocked = false;
            for (int i = ax_neg_start[k]; i < ax_neg_start[k + 1]; ++i) {
                if (test_bit(bits, ax_neg[i])) {
                    blocked = true;
                    break;
                }
            }
            if (blocked) {
                ax_unsatisfied[k] = -1;
                continue;
            }
            int count = 0;
            for (int i = ax_body_start[k]; i < ax_body_start[k + 1]; ++i) {
                if (!test_bit(bits, ax_body[i]))
                    ++count;
            }
            ax_unsatisfied[k] = count;
        }
        auto fire = [&](int k) {
            int h = ax_head[k];
            if (!test_bit(bits, h)) {
                set_bit(bits, h);
                ax_queue.push_back(h);
            }
        };
        for (int k = begin; k < end; ++k) {
            if (ax_unsatisfied[k] == 0)
                fire(k);
        }
        for (size_t q = 0; q < ax_queue.size(); ++q) {
            int h = ax_queue[q];
            for (int i = body_of_start[h]; i < body_of_start[h + 1]; ++i) {
                int k = body_of[i];
                if (ax_stratum[k] != stratum || ax_unsatisfied[k] <= 0)
                    continue;
                if (--ax_unsatisfied[k] == 0)
                    fire(k);
            }
        }
    }
}

void GroundSuccessorGenerator::apply(const uint64_t *parent_bits, int o, uint64_t *child,
                                     vector<int> &added, vector<int> &removed) {
    memcpy(child, parent_bits, sizeof(uint64_t) * num_words);
    const GroundOperator &op = task.operators[o];
    fired_ce.clear();
    for (int c : op.conditional_effects) {
        const GroundConditionalEffect &ce = task.conditional_effects[c];
        bool fires = true;
        for (int f : ce.condition) {
            if (!test_bit(parent_bits, f)) {
                fires = false;
                break;
            }
        }
        if (fires) {
            for (int f : ce.negated_condition) {
                if (test_bit(parent_bits, f)) {
                    fires = false;
                    break;
                }
            }
        }
        if (fires)
            fired_ce.push_back(c);
    }
    for (int f : op.del)
        reset_bit(child, f);
    for (int c : fired_ce) {
        for (int f : task.conditional_effects[c].del)
            reset_bit(child, f);
    }
    for (int f : op.add)
        set_bit(child, f);
    for (int c : fired_ce) {
        for (int f : task.conditional_effects[c].add)
            set_bit(child, f);
    }
    evaluate_axioms(child);
    for (int w = 0; w < num_words; ++w) {
        uint64_t word = child[w] & ~parent_bits[w];
        while (word) {
            int t = __builtin_ctzll(word);
            added.push_back(w * 64 + t);
            word &= word - 1;
        }
        word = parent_bits[w] & ~child[w];
        while (word) {
            int t = __builtin_ctzll(word);
            removed.push_back(w * 64 + t);
            word &= word - 1;
        }
    }
}


GroundedSearch::GroundedSearch(const Task &task, shared_ptr<GroundTask> gt,
                               const string &heuristic_name, bool full_novelty_check,
                               int width, bool use_novelty, bool use_ff, bool use_lm)
    : task(task), ground_task(std::move(gt)), heuristic_name(heuristic_name),
      full_novelty_check(full_novelty_check), width(width),
      use_novelty(use_novelty), use_ff(use_ff), use_lm(use_lm),
      compress_chains(!getenv("PWL_NO_CHAINS")),
      registry(ground_task->num_facts()), generator(*ground_task) {
    GroundedHeuristic::Type type = GroundedHeuristic::FF;
    if (heuristic_name == "add")
        type = GroundedHeuristic::ADD;
    else if (heuristic_name == "hmax")
        type = GroundedHeuristic::MAX;
    heuristic = make_unique<GroundedHeuristic>(task, ground_task, type);
    num_words = registry.get_num_words();
    goal_pos = ground_task->goal;
    const GoalCondition &goal = task.get_goal();
    for (const AtomicGoal &g : goal.goal) {
        if (!g.is_negated() || task.predicates[g.get_predicate_index()].isStaticPredicate())
            continue;
        int f = ground_task->get_fact(g.get_predicate_index(), g.get_arguments());
        if (f >= 0)
            goal_neg.push_back(f);
    }
    for (int p : goal.negative_nullary_goals) {
        int f = ground_task->get_fact(p, GroundAtom());
        if (f >= 0)
            goal_neg.push_back(f);
    }
}

GroundedSearch::~GroundedSearch() = default;

int GroundedSearch::count_unsatisfied_goals(const uint64_t *bits) const {
    int count = 0;
    for (int f : goal_pos) {
        if (!test_bit(bits, f))
            ++count;
    }
    for (int f : goal_neg) {
        if (test_bit(bits, f))
            ++count;
    }
    return count;
}

void GroundedSearch::extract_and_print_plan(int state_id) const {
    vector<int> ops;
    while (parent[state_id] >= 0) {
        int o = parent_op[state_id];
        if (o >= 0) {
            ops.push_back(o);
        }
        else {
            const vector<int> &chain = chains[-o - 2];
            for (auto it = chain.rbegin(); it != chain.rend(); ++it)
                ops.push_back(*it);
        }
        state_id = parent[state_id];
    }
    reverse(ops.begin(), ops.end());
    vector<LiftedOperatorId> plan;
    for (int o : ops) {
        const GroundOperator &op = ground_task->operators[o];
        vector<int> instantiation = op.instantiation;
        plan.emplace_back(op.schema, std::move(instantiation), unordered_map<int, int>());
    }
    print_plan(plan, task);
}

utils::ExitCode GroundedSearch::search() {
    cout << "Starting grounded search (queues:" << (use_novelty ? " novelty" : "")
         << (use_ff ? " ff" : "") << (use_lm ? " landmarks" : "") << ", width " << width
         << ")" << endl;
    clock_t timer_start = clock();

    if (ground_task->goal_unreachable) {
        cout << "Goal is not relaxed reachable." << endl;
        print_no_solution_found(timer_start);
        return utils::ExitCode::SEARCH_UNSOLVABLE;
    }

    vector<uint64_t> buffer(num_words, 0);
    {
        vector<int> init_facts;
        ground_task->get_state_facts(task.initial_state, init_facts);
        for (int f : init_facts)
            set_bit(buffer.data(), f);
        generator.evaluate_axioms(buffer.data());
    }
    int root = registry.insert(buffer.data()).first;

    vector<int> state_facts;
    collect_true_facts(registry.get(root), num_words, state_facts);
    int h0 = heuristic->compute_for_facts(state_facts, false);
    ++evaluated;
    cout << "Initial heuristic value: " << h0 << endl;

    // Relevant atoms: the atoms of the relaxed plan of the initial state.
    vector<int> relevant = heuristic->get_useful_facts();
    sort(relevant.begin(), relevant.end());
    cout << "Total number of relevant atoms: " << relevant.size() << endl;
    int num_goals = goal_pos.size() + goal_neg.size();
    // Relevant complement facts are satisfied iff their base fact is false.
    const int num_base_facts = heuristic->get_num_base_facts();
    vector<int> relevant_pos, relevant_neg;
    for (int f : relevant) {
        if (f < num_base_facts)
            relevant_pos.push_back(f);
        else
            relevant_neg.push_back(heuristic->get_complement_base(f));
    }
    auto count_unsatisfied_relevant = [&](const uint64_t *bits) {
        int count = 0;
        for (int f : relevant_pos) {
            if (!test_bit(bits, f))
                ++count;
        }
        for (int f : relevant_neg) {
            if (test_bit(bits, f))
                ++count;
        }
        return count;
    };

    // Landmarks.
    unique_ptr<LandmarkCountHeuristic> lm;
    int lm_words = 0;
    vector<uint64_t> accepted_store;
    if (use_lm) {
        vector<int> init_relaxed = state_facts;
        heuristic->add_complement_facts(state_facts, init_relaxed);
        lm = make_unique<LandmarkCountHeuristic>(*heuristic, init_relaxed, 200000000LL);
        if (!lm->is_ok()) {
            lm.reset();
            use_lm = false;
            if (!use_novelty && !use_ff)
                use_ff = true;
        }
        else {
            lm_words = lm->get_num_words();
        }
    }

    GroundNovelty novelty_evaluator(width, ground_task->num_facts(), num_goals);
    vector<int> child_facts;
    auto full_novelty = [&](const uint64_t *bits, int gc, int rc) {
        if (novelty_evaluator.get_width() > 1)
            collect_true_facts(bits, num_words, child_facts);
        return novelty_evaluator.full(bits, child_facts, gc, rc);
    };
    auto effects_novelty = [&](const uint64_t *bits, const vector<int> &added, int gc, int rc) {
        if (novelty_evaluator.get_width() > 1)
            collect_true_facts(bits, num_words, child_facts);
        return novelty_evaluator.effects(added, child_facts, gc, rc);
    };

    auto add_node = [&](int id) {
        if (id >= int(parent.size())) {
            parent.resize(id + 1, -1);
            parent_op.resize(id + 1, -1);
            g_value.resize(id + 1, 0);
            h_value.resize(id + 1, 0);
            num_unsat_goals.resize(id + 1, 0);
            num_unsat_relevant.resize(id + 1, 0);
            closed.resize(id + 1, 0);
            if (lm_words)
                accepted_store.resize(size_t(id + 1) * lm_words, 0);
        }
    };
    auto accepted_of = [&](int id) {
        return accepted_store.data() + size_t(id) * lm_words;
    };

    add_node(root);
    const uint64_t *root_bits = registry.get(root);
    num_unsat_goals[root] = count_unsatisfied_goals(root_bits);
    num_unsat_relevant[root] = count_unsatisfied_relevant(root_bits);
    h_value[root] = h0;
    full_novelty(root_bits, num_unsat_goals[root], num_unsat_relevant[root]);
    int h_lm0 = 0;
    if (lm) {
        lm->initial_accepted(root_bits, accepted_of(root));
        h_lm0 = lm->compute(accepted_of(root), root_bits);
        cout << "Initial landmark heuristic value: " << h_lm0 << endl;
    }
    if (num_unsat_goals[root] == 0) {
        print_goal_time(timer_start);
        extract_and_print_plan(root);
        return utils::ExitCode::SUCCESS;
    }
    if (h0 == UNSOLVABLE_STATE) {
        print_no_solution_found(timer_start);
        return utils::ExitCode::SEARCH_UNSOLVABLE;
    }

    // Open lists: one regular and one preferred list per queue type.
    enum {NOVELTY_QUEUE, FF_QUEUE, LM_QUEUE};
    vector<int> queue_types;
    if (use_novelty)
        queue_types.push_back(NOVELTY_QUEUE);
    if (use_ff)
        queue_types.push_back(FF_QUEUE);
    if (use_lm)
        queue_types.push_back(LM_QUEUE);
    int num_queues = queue_types.size();
    vector<KeyedOpenList<3>> regular(num_queues), preferred_lists(num_queues);
    size_t turn = 0;
    int priority = 1000;
    auto insert = [&](int id, int h_ff, int h_lm, int g, int gc, int novelty, bool preferred) {
        for (int q = 0; q < num_queues; ++q) {
            array<int, 3> key;
            if (queue_types[q] == NOVELTY_QUEUE)
                key = {novelty, gc, g};
            else if (queue_types[q] == FF_QUEUE)
                key = {h_ff, g, 0};
            else
                key = {h_lm, h_ff, 0};
            regular[q].push(key, id);
            if (preferred)
                preferred_lists[q].push(key, id);
        }
    };
    auto pop = [&]() {
        for (int k = 0; k < num_queues; ++k) {
            int q = (turn + k) % num_queues;
            if (regular[q].empty() && preferred_lists[q].empty())
                continue;
            turn = q + 1;
            if ((priority > 0 && !preferred_lists[q].empty()) || regular[q].empty()) {
                --priority;
                return preferred_lists[q].pop();
            }
            return regular[q].pop();
        }
        return -1;
    };
    insert(root, h0, h_lm0, 0, num_unsat_goals[root], 1, false);

    int best_h = h0;
    int best_h_lm = h_lm0;
    vector<int> applicable;
    vector<int> added, removed;
    // Indexed by base fact. Bit 1: the fact is in the relaxed plan, bit 2:
    // its complement is; bit 4: the fact is an interesting landmark, bit 8:
    // its complement is.
    vector<char> marks(ground_task->num_facts(), 0);
    vector<int> marked, interesting;
    vector<uint64_t> child(num_words), chain_child(num_words);
    vector<int> chain, chain_facts, chain_ops, chain_added, chain_removed;
    vector<uint64_t> child_accepted(max(lm_words, 1)), chain_accepted(max(lm_words, 1));
    vector<uint64_t> parent_accepted(max(lm_words, 1));

    while (true) {
        int sid = pop();
        if (sid < 0)
            break;
        if (closed[sid])
            continue;
        closed[sid] = 1;
        ++expanded;
        if ((expanded & 0xFFFF) == 0) {
            cout << "[expanded " << expanded << ", registered " << registry.size()
                 << ", best h " << best_h << ", t="
                 << double(clock() - timer_start) / CLOCKS_PER_SEC << "s, "
                 << utils::get_peak_memory_in_kb() << " KB]" << endl;
        }

        // Copy: the registry may grow while generating successors.
        memcpy(buffer.data(), registry.get(sid), sizeof(uint64_t) * num_words);
        collect_true_facts(buffer.data(), num_words, state_facts);
        int h = heuristic->compute_for_facts(state_facts, false);
        ++evaluated;
        if (h == UNSOLVABLE_STATE)
            continue;
        h_value[sid] = h;
        bool progress = false;
        if (h < best_h) {
            best_h = h;
            progress = true;
        }
        if (lm) {
            memcpy(parent_accepted.data(), accepted_of(sid), sizeof(uint64_t) * lm_words);
            int h_lm = lm->compute(parent_accepted.data(), buffer.data());
            if (h_lm < best_h_lm) {
                best_h_lm = h_lm;
                progress = true;
            }
        }
        if (progress) {
            priority += 1000;
            cout << "New heuristic value expanded: h=" << best_h;
            if (lm)
                cout << ", h_lm=" << best_h_lm;
            cout << " [expansions: " << expanded << ", evaluations: " << evaluated
                 << ", generations: " << generated << ", time: "
                 << double(clock() - timer_start) / CLOCKS_PER_SEC << "]" << endl;
        }
        marked.clear();
        for (int f : heuristic->get_useful_facts()) {
            if (f < num_base_facts) {
                marks[f] |= 1;
                marked.push_back(f);
            }
            else {
                int b = heuristic->get_complement_base(f);
                marks[b] |= 2;
                marked.push_back(b);
            }
        }
        if (lm) {
            lm->get_interesting_facts(parent_accepted.data(), buffer.data(), interesting);
            for (int f : interesting) {
                if (f < num_base_facts) {
                    marks[f] |= 4;
                    marked.push_back(f);
                }
                else {
                    int b = heuristic->get_complement_base(f);
                    marks[b] |= 8;
                    marked.push_back(b);
                }
            }
        }

        generator.get_applicable(buffer.data(), state_facts, applicable);
        generated += applicable.size();
        int g = g_value[sid];
        int parent_gc = num_unsat_goals[sid];
        int parent_rc = num_unsat_relevant[sid];
        for (int o : applicable) {
            added.clear();
            removed.clear();
            generator.apply(buffer.data(), o, child.data(), added, removed);
            int dist = g + ground_task->operators[o].cost;
            int op_ref = o;
            if (lm)
                lm->update_accepted(parent_accepted.data(), child.data(), child_accepted.data());
            if (compress_chains) {
                /*
                  Follow forced chains: while the successor has exactly one
                  applicable operator (and is not a goal), apply it right
                  away, without evaluating or registering the intermediate
                  states. Chains ending in a state without applicable
                  operators (a dead end) are pruned.
                */
                chain.clear();
                bool dead_end = false;
                for (int steps = 0; steps < 10000; ++steps) {
                    if (count_unsatisfied_goals(child.data()) == 0)
                        break;
                    collect_true_facts(child.data(), num_words, chain_facts);
                    generator.get_applicable(child.data(), chain_facts, chain_ops);
                    if (chain_ops.empty()) {
                        dead_end = true;
                        break;
                    }
                    if (chain_ops.size() != 1)
                        break;
                    if (chain.empty())
                        chain.push_back(o);
                    int next = chain_ops[0];
                    chain_added.clear();
                    chain_removed.clear();
                    generator.apply(child.data(), next, chain_child.data(),
                                    chain_added, chain_removed);
                    child.swap(chain_child);
                    chain.push_back(next);
                    dist += ground_task->operators[next].cost;
                    if (lm) {
                        lm->update_accepted(child_accepted.data(), child.data(),
                                            chain_accepted.data());
                        child_accepted.swap(chain_accepted);
                    }
                }
                if (dead_end) {
                    ++pruned_dead_ends;
                    continue;
                }
                if (!chain.empty()) {
                    ++compressed_chains;
                    chain_length_sum += chain.size();
                    added.clear();
                    removed.clear();
                    for (int w = 0; w < num_words; ++w) {
                        uint64_t word = child[w] & ~buffer[w];
                        while (word) {
                            added.push_back(w * 64 + __builtin_ctzll(word));
                            word &= word - 1;
                        }
                        word = buffer[w] & ~child[w];
                        while (word) {
                            removed.push_back(w * 64 + __builtin_ctzll(word));
                            word &= word - 1;
                        }
                    }
                    op_ref = -int(chains.size()) - 2;
                }
            }
            auto res = registry.insert(child.data());
            int cid = res.first;
            if (op_ref < -1 && (res.second || dist < g_value[cid]))
                chains.push_back(chain);
            if (!res.second) {
                if (dist < g_value[cid]) {
                    g_value[cid] = dist;
                    parent[cid] = sid;
                    parent_op[cid] = op_ref;
                    if (closed[cid]) {
                        closed[cid] = 0;
                        int novelty = full_novelty(child.data(), num_unsat_goals[cid],
                                                   num_unsat_relevant[cid]);
                        int h_lm = lm ? lm->compute(accepted_of(cid), child.data()) : 0;
                        insert(cid, h, h_lm, dist, num_unsat_goals[cid], novelty, false);
                    }
                }
                continue;
            }
            add_node(cid);
            parent[cid] = sid;
            parent_op[cid] = op_ref;
            g_value[cid] = dist;
            h_value[cid] = h;
            int gc = count_unsatisfied_goals(child.data());
            int rc = count_unsatisfied_relevant(child.data());
            num_unsat_goals[cid] = gc;
            num_unsat_relevant[cid] = rc;
            if (gc == 0) {
                print_goal_time(timer_start);
                extract_and_print_plan(cid);
                return utils::ExitCode::SUCCESS;
            }
            int novelty = 0;
            if (use_novelty) {
                if (!full_novelty_check && gc == parent_gc && rc == parent_rc)
                    novelty = effects_novelty(child.data(), added, gc, rc);
                else
                    novelty = full_novelty(child.data(), gc, rc);
            }
            int h_lm = 0;
            if (lm) {
                memcpy(accepted_of(cid), child_accepted.data(), sizeof(uint64_t) * lm_words);
                h_lm = lm->compute(accepted_of(cid), child.data());
            }
            bool preferred = false;
            for (int f : added) {
                if (marks[f] & 5) {
                    preferred = true;
                    break;
                }
            }
            if (!preferred) {
                for (int f : removed) {
                    if (marks[f] & 10) {
                        preferred = true;
                        break;
                    }
                }
            }
            insert(cid, h, h_lm, dist, gc, novelty, preferred);
        }
        for (int f : marked)
            marks[f] = 0;
    }

    print_no_solution_found(timer_start);
    return utils::ExitCode::SEARCH_UNSOLVABLE;
}

void GroundedSearch::print_statistics() const {
    cout << "Expanded " << expanded << " state(s)." << endl;
    cout << "Evaluated " << evaluated << " state(s)." << endl;
    cout << "Generated " << generated << " state(s)." << endl;
    cout << "Registered " << registry.size() << " state(s)." << endl;
    if (compress_chains) {
        cout << "Compressed chains: " << compressed_chains << " (average length "
             << (compressed_chains ? double(chain_length_sum) / compressed_chains : 0.0)
             << "), pruned dead ends: " << pruned_dead_ends << endl;
    }
    cout << "State registry memory: " << registry.memory_usage_bytes() / 1024 << " KB" << endl;
}

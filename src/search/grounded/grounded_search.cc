#include "grounded_search.h"

#include "../action.h"
#include "../task.h"

#include "../grounding/ground_task.h"
#include "../heuristics/grounded_heuristic.h"
#include "../search_engines/utils.h"

#include "landmarks.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <iostream>
#include <limits>
#include <malloc.h>
#include <map>
#include <random>
#include <unordered_map>
#include <unistd.h>

using namespace std;

namespace ground_search_detail {
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

    // Memory used by pair tables; beyond the limit, new partitions only get
    // width-1 tables.
    size_t pair_memory = 0;
    static const size_t PAIR_MEMORY_LIMIT = size_t(512) * 1024 * 1024;

    Table &get_table(int gc, int rc) {
        long long key = (long long)rc * (num_goals + 1) + gc;
        auto it = tables.find(key);
        if (it == tables.end()) {
            Table t;
            t.singles.assign(num_words, 0);
            size_t pair_words = (size_t(num_facts) * num_facts + 63) / 64;
            if (width > 1 && pair_memory + pair_words * 8 <= PAIR_MEMORY_LIMIT) {
                t.pairs.assign(pair_words, 0);
                pair_memory += pair_words * 8;
            }
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
    static const int MAX_FACTS_FOR_WIDTH_2 = 4096;

    GroundNovelty(int width, int num_facts, int num_goals)
        : width(width), num_facts(num_facts), num_words((num_facts + 63) / 64),
          num_goals(num_goals) {
        if (num_words == 0)
            num_words = 1;
        int max_facts = getenv("PWL_MAX_W2_FACTS") ? atoi(getenv("PWL_MAX_W2_FACTS"))
                                                   : MAX_FACTS_FOR_WIDTH_2;
        if (width > 1 && num_facts > max_facts) {
            cout << "Too many facts for width-2 novelty; using width 1." << endl;
            this->width = 1;
        }
    }

    int get_width() const {
        return width;
    }

    size_t memory_usage() const {
        size_t bytes = 0;
        for (const auto &entry : tables)
            bytes += (entry.second.singles.capacity() + entry.second.pairs.capacity()) *
                sizeof(uint64_t);
        return bytes;
    }

    //! Novelty of the state with the given bits (and, for width 2, true
    //! facts in increasing order).
    int full(const uint64_t *bits, const vector<int> &facts, int gc, int rc) {
        ++checks;
        if (width > 1)
            pair_work += (long long)facts.size() * facts.size() / 2;
        Table &t = get_table(gc, rc);
        bool novel = false;
        for (int w = 0; w < num_words; ++w) {
            if (bits[w] & ~t.singles[w]) {
                novel = true;
                t.singles[w] |= bits[w];
            }
        }
        if (width == 1 || t.pairs.empty())
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
    //! Work of the width-2 checks (pairs considered) and number of checks.
    long long pair_work = 0;
    long long checks = 0;

    //! Use width 1 from now on (freeing the pair tables).
    void downgrade_to_width_1() {
        width = 1;
        for (auto &entry : tables)
            vector<uint64_t>().swap(entry.second.pairs);
        pair_memory = 0;
    }

    int effects(const vector<int> &added, const vector<int> &facts, int gc, int rc) {
        ++checks;
        if (width > 1)
            pair_work += (long long)added.size() * facts.size();
        Table &t = get_table(gc, rc);
        bool novel = false;
        for (int f : added) {
            if (test_and_set(t.singles, f))
                novel = true;
        }
        if (width == 1 || t.pairs.empty())
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
}  // namespace ground_search_detail

using namespace ground_search_detail;


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

    /*
      Watch every operator by the precondition that is least likely to be
      true: the one whose predicate has the smallest fraction of facts true
      in the initial state, then the one occurring in the fewest operator
      preconditions.
    */
    int num_facts = task.num_facts();
    vector<int> occurrences(num_facts, 0);
    for (int f : pre)
        ++occurrences[f];
    int num_predicates = 0;
    for (const auto &fact : task.facts)
        num_predicates = max(num_predicates, fact.first + 1);
    vector<int> facts_of_pred(num_predicates, 0), true_of_pred(num_predicates, 0);
    for (const auto &fact : task.facts)
        ++facts_of_pred[fact.first];
    for (int f : task.initial_facts)
        ++true_of_pred[task.facts[f].first];
    auto true_fraction = [&](int f) {
        int p = task.facts[f].first;
        return double(true_of_pred[p]) / facts_of_pred[p];
    };
    vector<vector<int>> watched_by(num_facts);
    for (size_t o = 0; o < ops.size(); ++o) {
        if (ops[o].pre.empty()) {
            no_pre_ops.push_back(o);
            continue;
        }
        int best = ops[o].pre[0];
        for (int f : ops[o].pre) {
            double tf = true_fraction(f), tb = true_fraction(best);
            if (tf < tb || (tf == tb && occurrences[f] < occurrences[best]))
                best = f;
        }
        watched_by[best].push_back(o);
    }
    vector<const vector<int> *> watch_lists;
    for (const auto &l : watched_by)
        watch_lists.push_back(&l);
    build_csr(watch_lists, watch_start, watch);
    compute_watch_mask();

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

/*
  Second level of the watch index: the operators watched by a fact f are
  grouped by a second precondition g (the one occurring in the fewest of
  these operators), so that only groups whose fact g is true are scanned.
*/
void GroundSuccessorGenerator::compute_watch_mask() {
    watch_mask.assign(num_words, 0);
    for (size_t f = 0; f + 1 < watch_start.size(); ++f) {
        if (watch_start[f + 1] > watch_start[f])
            set_bit(watch_mask.data(), f);
    }

    group_start.clear();
    group_fact.clear();
    group_op_start.clear();
    group_ops.clear();
    int num_facts = watch_start.size() - 1;
    vector<int> count(num_facts, 0);
    vector<int> touched;
    vector<pair<int, int>> keyed;
    for (int f = 0; f < num_facts; ++f) {
        group_start.push_back(group_fact.size());
        int begin = watch_start[f], end = watch_start[f + 1];
        if (end - begin <= 16) {
            if (end > begin) {
                group_fact.push_back(-1);
                group_op_start.push_back(group_ops.size());
                group_ops.insert(group_ops.end(), watch.begin() + begin, watch.begin() + end);
            }
            continue;
        }
        touched.clear();
        for (int i = begin; i < end; ++i) {
            int o = watch[i];
            for (int k = pre_start[o]; k < pre_start[o + 1]; ++k) {
                int g = pre[k];
                if (g == f)
                    continue;
                if (count[g]++ == 0)
                    touched.push_back(g);
            }
        }
        keyed.clear();
        for (int i = begin; i < end; ++i) {
            int o = watch[i];
            int best = -1;
            for (int k = pre_start[o]; k < pre_start[o + 1]; ++k) {
                int g = pre[k];
                if (g != f && (best < 0 || count[g] < count[best]))
                    best = g;
            }
            keyed.emplace_back(best, o);
        }
        for (int g : touched)
            count[g] = 0;
        sort(keyed.begin(), keyed.end());
        for (size_t i = 0; i < keyed.size(); ++i) {
            if (i == 0 || keyed[i].first != keyed[i - 1].first) {
                group_fact.push_back(keyed[i].first);
                group_op_start.push_back(group_ops.size());
            }
            group_ops.push_back(keyed[i].second);
        }
    }
    group_start.push_back(group_fact.size());
    group_op_start.push_back(group_ops.size());

    /*
      Facts watching many groups get a bitset of their groups' facts: ANDed
      with the state, it yields the groups to scan; a group's index is the
      rank of its fact in the bitset (groups are sorted by fact).
    */
    mask_start.assign(num_facts, -1);
    group_masks.clear();
    const size_t max_mask_words = size_t(16) * 1024 * 1024;
    for (int f = 0; f < num_facts; ++f) {
        int first = group_start[f], last = group_start[f + 1];
        if (first < last && group_fact[first] < 0)
            ++first;
        if (last - first <= 2 * num_words ||
            group_masks.size() + num_words > max_mask_words)
            continue;
        mask_start[f] = group_masks.size();
        group_masks.resize(group_masks.size() + num_words, 0);
        for (int i = first; i < last; ++i)
            set_bit(group_masks.data() + mask_start[f], group_fact[i]);
    }
}

void GroundSuccessorGenerator::restrict_operators(const vector<char> &allowed) {
    vector<int> new_watch_start, new_watch;
    for (size_t f = 0; f + 1 < watch_start.size(); ++f) {
        new_watch_start.push_back(new_watch.size());
        for (int i = watch_start[f]; i < watch_start[f + 1]; ++i) {
            if (allowed[watch[i]])
                new_watch.push_back(watch[i]);
        }
    }
    new_watch_start.push_back(new_watch.size());
    watch_start.swap(new_watch_start);
    watch.swap(new_watch);
    compute_watch_mask();
    vector<int> new_no_pre;
    for (int o : no_pre_ops) {
        if (allowed[o])
            new_no_pre.push_back(o);
    }
    no_pre_ops.swap(new_no_pre);
}

bool GroundSuccessorGenerator::is_applicable(const uint64_t *bits, int o) const {
    for (int i = pre_start[o]; i < pre_start[o + 1]; ++i) {
        if (!test_bit(bits, pre[i]))
            return false;
    }
    for (int i = neg_start[o]; i < neg_start[o + 1]; ++i) {
        if (test_bit(bits, neg[i]))
            return false;
    }
    return true;
}

void GroundSuccessorGenerator::get_applicable(const uint64_t *bits,
                                              const vector<int> &true_facts,
                                              vector<int> &ops, size_t max_ops) const {
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
        if (check(o)) {
            ops.push_back(o);
            if (ops.size() >= max_ops)
                return;
        }
    }
    (void)true_facts;
    for (int w = 0; w < num_words; ++w) {
        uint64_t word = bits[w] & watch_mask[w];
        while (word) {
            int f = w * 64 + __builtin_ctzll(word);
            word &= word - 1;
            auto scan_group = [&](int i) {
                for (int k = group_op_start[i]; k < group_op_start[i + 1]; ++k) {
                    int o = group_ops[k];
                    if (check(o))
                        ops.push_back(o);
                }
            };
            if (ops.size() >= max_ops)
                return;
            if (mask_start[f] >= 0) {
                int i = group_start[f];
                if (i < group_start[f + 1] && group_fact[i] < 0)
                    scan_group(i++);
                const uint64_t *mask = group_masks.data() + mask_start[f];
                int rank = i;
                for (int mw = 0; mw < num_words; ++mw) {
                    uint64_t m = mask[mw];
                    if (!m)
                        continue;
                    uint64_t hit = m & bits[mw];
                    while (hit) {
                        int b = __builtin_ctzll(hit);
                        hit &= hit - 1;
                        scan_group(rank + __builtin_popcountll(m & ((1ULL << b) - 1)));
                    }
                    rank += __builtin_popcountll(m);
                }
                continue;
            }
            for (int i = group_start[f]; i < group_start[f + 1]; ++i) {
                int g = group_fact[i];
                if (g >= 0 && !test_bit(bits, g))
                    continue;
                scan_group(i);
            }
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


/*
  One search run: the open lists, novelty tables, state registry, and search
  nodes of one configuration. step() expands states until its time slice is
  used up, so that runs can be interleaved.
*/
class GroundSearchRun {
public:
    enum Status {RUNNING, SOLVED, EXHAUSTED};

private:
    GroundedSearch &ctx;
    GroundSearchConfig config;
    const GroundTask &task;
    GroundedHeuristic &heuristic;
    const LandmarkCountHeuristic *lm;
    int num_words;
    int num_base_facts;
    int lm_words = 0;

    GroundStateRegistry registry;
    GroundNovelty novelty;
    //! Lazy novelty open list (an extra queue): novelty of the expanded
    //! state in its own tables, inherited by the successors.
    bool lazy_queue;
    GroundNovelty lazy_table;
    int parent_lazy_novelty = 1;
    mt19937 rng;

    // Search nodes, indexed by state id.
    vector<int> parent;
    vector<int> parent_op;
    vector<int> g_value;
    vector<short> num_unsat_goals;
    vector<int> num_unsat_relevant;
    vector<char> closed;
    vector<uint64_t> accepted_store;
    //! Operator sequences of compressed chains; parent_op[s] = -k - 2 refers
    //! to chains[k].
    vector<vector<int>> chains;

    enum {NOVELTY_QUEUE, FF_QUEUE, ADD_QUEUE, LM_QUEUE, LAZY_NOVELTY_QUEUE, G_QUEUE};
    const vector<int> empty_facts;
    vector<int> queue_types;
    vector<KeyedOpenList<3>> regular, preferred_lists;
    size_t turn = 0;
    int priority = 1000;
    int h_add = 0;
    //! Break ties in the novelty open list by h instead of g.
    bool novelty_tiebreak_h = getenv("PWL_NOVELTY_TIEBREAK_H") != nullptr;
    //! Partition novelty by the landmark count (requires landmarks).
    bool novelty_by_lm = false;
    bool nolan_lists = getenv("PWL_NOLAN_LISTS") != nullptr;
    //! Novelty of a state is computed when it is expanded and inherited by
    //! its successors (as in lazy search).
    bool lazy_novelty = config.lazy_novelty || getenv("PWL_LAZY_NOVELTY") != nullptr;
    int parent_novelty = 1;
    int parent_h_lm = 0;
    //! Preferred successors: generated by relaxed plan operators (instead of
    //! achieving a relaxed plan atom).
    bool helpful_actions = getenv("PWL_HELPFUL_ACTIONS") != nullptr;
    vector<char> in_relaxed_plan;
    //! Relaxed plan lookahead (disabled with PWL_NO_LOOKAHEAD).
    bool lookahead = getenv("PWL_NO_LOOKAHEAD") == nullptr;
    bool lookahead_not_preferred = getenv("PWL_LOOKAHEAD_REGULAR") != nullptr;
    vector<int> lookahead_order;
    int best_h, best_h_add, best_h_lm;
    int goal_state = -1;

    // Scratch data.
    vector<uint64_t> buffer, child, chain_child, cycle_ref;
    vector<int> state_facts, child_facts, applicable, added, removed;
    vector<int> chain, chain_facts, chain_ops, chain_added, chain_removed;
    vector<uint64_t> child_accepted, chain_accepted, parent_accepted;
    // Indexed by base fact. Bit 1: the fact is in the relaxed plan, bit 2:
    // its complement is; bit 4: the fact is an interesting landmark, bit 8:
    // its complement is.
    vector<char> marks;
    vector<int> marked, interesting;

public:
    long long expanded = 0;
    long long generated = 0;
    long long evaluated = 0;
    long long compressed_chains = 0;
    long long chain_length_sum = 0;
    long long pruned_dead_ends = 0;
    long long pruned_by_novelty = 0;
    long long lookahead_states = 0;
    double time_used = 0;
    long long trace_expansions = getenv("PWL_TRACE_EXPAND") ? atoll(getenv("PWL_TRACE_EXPAND")) : 0;
    long long step_limit = getenv("PWL_EXPANSION_LIMIT") ? atoll(getenv("PWL_EXPANSION_LIMIT")) : -1;

private:
    void add_node(int id) {
        if (id >= int(parent.size())) {
            parent.resize(id + 1, -1);
            parent_op.resize(id + 1, -1);
            g_value.resize(id + 1, 0);
            num_unsat_goals.resize(id + 1, 0);
            num_unsat_relevant.resize(id + 1, 0);
            closed.resize(id + 1, 0);
            if (lm_words)
                accepted_store.resize(size_t(id + 1) * lm_words, 0);
        }
    }

    uint64_t *accepted_of(int id) {
        return accepted_store.data() + size_t(id) * lm_words;
    }

    int full_novelty(const uint64_t *bits, int gc, int rc) {
        if (novelty.get_width() > 1)
            collect_true_facts(bits, num_words, child_facts);
        return novelty.full(bits, child_facts, gc, rc);
    }

    int effects_novelty(const uint64_t *bits, int gc, int rc) {
        if (novelty.get_width() > 1)
            collect_true_facts(bits, num_words, child_facts);
        return novelty.effects(added, child_facts, gc, rc);
    }

    void insert(int id, int h_ff, int h_lm, int g, int gc, int nov, bool preferred) {
        for (size_t q = 0; q < queue_types.size(); ++q) {
            array<int, 3> key;
            if (queue_types[q] == NOVELTY_QUEUE)
                key = {nov, novelty_by_lm ? (lazy_novelty ? parent_h_lm : h_lm) : gc,
                       novelty_tiebreak_h ? h_ff : g};
            else if (queue_types[q] == FF_QUEUE)
                key = {h_ff, g, 0};
            else if (queue_types[q] == ADD_QUEUE)
                key = {h_add, g, 0};
            else if (queue_types[q] == G_QUEUE)
                key = {g, 0, 0};
            else if (queue_types[q] == LAZY_NOVELTY_QUEUE)
                key = {parent_lazy_novelty, novelty_by_lm ? parent_h_lm : gc, g};
            else
                key = {h_lm, h_ff, 0};
            // NOLAN's list structure: no preferred novelty list, and only a
            // preferred landmark list.
            if (!(nolan_lists && queue_types[q] == LM_QUEUE))
                regular[q].push(key, id);
            if (preferred && !(nolan_lists && queue_types[q] == NOVELTY_QUEUE))
                preferred_lists[q].push(key, id);
        }
    }

    int pop() {
        int num_queues = queue_types.size();
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
    }

    void expand(int sid);
    bool process_child(int sid, int o, int op_ref, int dist, int h, int parent_gc,
                       int parent_rc, bool force_preferred);
    void relaxed_plan_lookahead(int sid, int g, int h, int parent_gc, int parent_rc);

public:
    GroundSearchRun(GroundedSearch &ctx, const GroundSearchConfig &config)
        : ctx(ctx), config(config), task(*ctx.ground_task), heuristic(*ctx.heuristic),
          lm(config.lm ? ctx.landmarks.get() : nullptr), num_words(ctx.num_words),
          num_base_facts(ctx.heuristic->get_num_base_facts()),
          registry(ctx.ground_task->num_facts()),
          novelty(config.width, ctx.ground_task->num_facts(),
                  ctx.goal_pos.size() + ctx.goal_neg.size()),
          lazy_queue(config.novelty && (config.lazy_queue || getenv("PWL_LAZY_QUEUE"))),
          lazy_table(lazy_queue ? config.width : 1, lazy_queue ? ctx.ground_task->num_facts() : 0,
                     ctx.goal_pos.size() + ctx.goal_neg.size()),
          rng(config.seed) {
        if (config.novelty)
            queue_types.push_back(NOVELTY_QUEUE);
        if (lazy_queue)
            queue_types.push_back(LAZY_NOVELTY_QUEUE);
        if (config.ff)
            queue_types.push_back(FF_QUEUE);
        if (config.add)
            queue_types.push_back(ADD_QUEUE);
        if (lm)
            queue_types.push_back(LM_QUEUE);
        if (config.breadth_first)
            queue_types.assign(1, G_QUEUE);
        if (queue_types.empty())
            queue_types.push_back(FF_QUEUE);
        regular.resize(queue_types.size());
        preferred_lists.resize(queue_types.size());
        if (lm)
            lm_words = lm->get_num_words();
        novelty_by_lm = config.novelty_by_lm && lm;

        buffer.assign(num_words, 0);
        child.assign(num_words, 0);
        chain_child.assign(num_words, 0);
        child_accepted.assign(max(lm_words, 1), 0);
        chain_accepted.assign(max(lm_words, 1), 0);
        parent_accepted.assign(max(lm_words, 1), 0);
        marks.assign(task.num_facts(), 0);

        int root = registry.insert(ctx.initial_bits.data()).first;
        add_node(root);
        const uint64_t *root_bits = registry.get(root);
        num_unsat_goals[root] = ctx.count_unsatisfied_goals(root_bits);
        num_unsat_relevant[root] = ctx.count_unsatisfied_relevant(root_bits);
        int h_lm0 = 0;
        if (lm) {
            lm->initial_accepted(root_bits, accepted_of(root));
            h_lm0 = lm->compute(accepted_of(root), root_bits);
        }
        if (novelty_by_lm)
            full_novelty(root_bits, 0, h_lm0);
        else
            full_novelty(root_bits, num_unsat_goals[root], num_unsat_relevant[root]);
        best_h = ctx.initial_h;
        best_h_add = numeric_limits<int>::max();
        best_h_lm = h_lm0;
        insert(root, ctx.initial_h, h_lm0, 0, num_unsat_goals[root], 1, false);
        cout << "Search run " << config.name << ": queues"
             << (config.novelty ? " novelty" : "") << (config.ff ? " ff" : "")
             << (config.add ? " add" : "") << (lm ? " landmarks" : "")
             << ", width " << novelty.get_width() << (config.shuffle ? ", shuffled" : "")
             << endl;
    }

    const string &get_name() const {
        return config.name;
    }

    //! Whether exhausting the open lists proves unsolvability.
    bool is_complete() const {
        return config.prune_novelty <= 0 && config.max_expansions < 0;
    }

    size_t memory_usage() const {
        return registry.memory_usage_bytes() + novelty.memory_usage() +
            parent.capacity() * (3 * sizeof(int) + sizeof(short) + sizeof(int) + 1) +
            accepted_store.capacity() * sizeof(uint64_t);
    }

    Status step(double time_slice) {
        clock_t start = clock();
        while (true) {
            int sid = pop();
            if (sid < 0)
                return EXHAUSTED;
            if (closed[sid])
                continue;
            closed[sid] = 1;
            ++expanded;
            expand(sid);
            if (expanded == 32 || expanded == 256)
                check_pair_work();
            if (goal_state >= 0)
                return SOLVED;
            if (config.max_expansions >= 0 && expanded >= config.max_expansions)
                return EXHAUSTED;
            if ((expanded & 0xFFFF) == 0) {
                cout << "[" << config.name << ": expanded " << expanded << ", registered "
                     << registry.size() << ", best h " << best_h << ", "
                     << utils::get_peak_memory_in_kb() << " KB]" << endl;
            }
            if ((expanded & 0xF) == 0 &&
                (double(clock() - start) / CLOCKS_PER_SEC > time_slice ||
                 (step_limit > 0 && expanded >= step_limit)))
                return RUNNING;
        }
    }

    void get_plan(vector<int> &ops) const {
        int state_id = goal_state;
        while (parent[state_id] >= 0) {
            int o = parent_op[state_id];
            if (o >= 0) {
                ops.push_back(o);
            }
            else {
                const vector<int> &c = chains[-o - 2];
                for (auto it = c.rbegin(); it != c.rend(); ++it)
                    ops.push_back(*it);
            }
            state_id = parent[state_id];
        }
        reverse(ops.begin(), ops.end());
    }

    size_t num_registered() const {
        return registry.size();
    }

    double get_start_time() const {
        return config.start_time;
    }

    /*
      Width-2 novelty checks cost |added facts| * |true facts| (or |true
      facts|^2 / 2) pair updates. If they cost too much per check (e.g.,
      chains changing many facts in states with many true facts), use width
      1 from now on.
    */
    void check_pair_work() {
        const long long max_pairs_per_check = 100000;
        for (GroundNovelty *table : {&novelty, &lazy_table}) {
            if (table->get_width() > 1 && table->checks > 0 &&
                table->pair_work / table->checks > max_pairs_per_check) {
                cout << "[" << config.name << "] Width-2 novelty too expensive ("
                     << table->pair_work / table->checks << " pairs per check); using width 1."
                     << endl;
                table->downgrade_to_width_1();
            }
        }
    }
};

void GroundSearchRun::expand(int sid) {
    // Copy: the registry may grow while generating successors.
    memcpy(buffer.data(), registry.get(sid), sizeof(uint64_t) * num_words);
    collect_true_facts(buffer.data(), num_words, state_facts);
    int h = 0;
    if (!config.breadth_first) {
        h = heuristic.compute_for_facts(state_facts, false);
        ++evaluated;
        if (h == UNSOLVABLE_STATE)
            return;
        h_add = heuristic.get_last_hadd();
    }
    if (trace_expansions > 0) {
        --trace_expansions;
        cout << "Expand " << sid << " g=" << g_value[sid] << " h=" << h;
        if (lm)
            cout << " h_lm=" << lm->compute(accepted_of(sid), buffer.data());
        cout << " op=" << parent_op[sid] << endl;
    }
    bool progress = false;
    if (h < best_h) {
        best_h = h;
        progress = true;
    }
    if (config.add && h_add < best_h_add) {
        best_h_add = h_add;
        progress = true;
    }
    if (lm) {
        memcpy(parent_accepted.data(), accepted_of(sid), sizeof(uint64_t) * lm_words);
        int h_lm = lm->compute(parent_accepted.data(), buffer.data());
        parent_h_lm = h_lm;
        if (h_lm < best_h_lm) {
            best_h_lm = h_lm;
            progress = true;
        }
    }
    if (lazy_queue) {
        if (lazy_table.get_width() > 1)
            collect_true_facts(buffer.data(), num_words, child_facts);
        parent_lazy_novelty = novelty_by_lm
            ? lazy_table.full(buffer.data(), child_facts, 0, parent_h_lm)
            : lazy_table.full(buffer.data(), child_facts, num_unsat_goals[sid],
                              num_unsat_relevant[sid]);
    }
    if (lazy_novelty && config.novelty) {
        // Lazy novelty: novelty of the expanded state, inherited by its
        // successors in the novelty open list.
        if (novelty_by_lm)
            parent_novelty = full_novelty(buffer.data(), 0, parent_h_lm);
        else
            parent_novelty = full_novelty(buffer.data(), num_unsat_goals[sid],
                                          num_unsat_relevant[sid]);
    }
    if (progress) {
        priority += 1000;
        cout << "[" << config.name << "] New heuristic value expanded: h=" << best_h;
        if (config.add)
            cout << ", h_add=" << best_h_add;
        if (lm)
            cout << ", h_lm=" << best_h_lm;
        cout << " [expansions: " << expanded << ", evaluations: " << evaluated
             << ", generations: " << generated << "]" << endl;
    }
    marked.clear();
    for (int f : config.breadth_first ? empty_facts : heuristic.get_useful_facts()) {
        if (f < num_base_facts) {
            marks[f] |= 1;
            marked.push_back(f);
        }
        else {
            int b = heuristic.get_complement_base(f);
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
                int b = heuristic.get_complement_base(f);
                marks[b] |= 8;
                marked.push_back(b);
            }
        }
    }

    if (helpful_actions) {
        if (in_relaxed_plan.empty())
            in_relaxed_plan.assign(task.operators.size(), 0);
        for (int a : heuristic.get_relaxed_plan())
            in_relaxed_plan[a] = 1;
    }
    GroundSuccessorGenerator &generator = ctx.generator;
    generator.get_applicable(buffer.data(), state_facts, applicable);
    if (config.shuffle)
        shuffle(applicable.begin(), applicable.end(), rng);
    generated += applicable.size();
    int g = g_value[sid];
    int parent_gc = num_unsat_goals[sid];
    int parent_rc = num_unsat_relevant[sid];
    for (int o : applicable) {
        added.clear();
        removed.clear();
        generator.apply(buffer.data(), o, child.data(), added, removed);
        int dist = g + task.operators[o].cost;
        int op_ref = o;
        if (lm)
            lm->update_accepted(parent_accepted.data(), child.data(), child_accepted.data());
        if (ctx.compress_chains) {
            /*
              Follow forced chains: while the successor has exactly one
              applicable operator (and is not a goal), apply it right away,
              without evaluating or registering the intermediate states.
              Chains ending in a state without applicable operators (a dead
              end) are pruned.
            */
            chain.clear();
            bool dead_end = false;
            // Brent's cycle detection: a forced cycle never reaches a goal.
            cycle_ref = child;
            int power = 1, length = 0;
            for (int steps = 0; steps < 10000; ++steps) {
                if (ctx.count_unsatisfied_goals(child.data()) == 0)
                    break;
                // Only whether there are 0, 1, or more applicable operators
                // matters.
                generator.get_applicable(child.data(), chain_facts, chain_ops, 2);
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
                dist += task.operators[next].cost;
                if (lm) {
                    lm->update_accepted(child_accepted.data(), child.data(),
                                        chain_accepted.data());
                    child_accepted.swap(chain_accepted);
                }
                if (child == cycle_ref) {
                    dead_end = true;
                    break;
                }
                if (++length == power) {
                    cycle_ref = child;
                    power *= 2;
                    length = 0;
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
        if (!process_child(sid, o, op_ref, dist, h, parent_gc, parent_rc, false))
            return;
    }
    if (lookahead && h > 0 && !config.breadth_first)
        relaxed_plan_lookahead(sid, g, h, parent_gc, parent_rc);
    for (int f : marked)
        marks[f] = 0;
    if (helpful_actions) {
        for (int a : heuristic.get_relaxed_plan())
            in_relaxed_plan[a] = 0;
    }
}


/*
  Register the successor in `child` (reached from sid by op_ref with cost
  dist; added/removed/child_accepted describe it). Returns false if it is a
  goal state.
*/
bool GroundSearchRun::process_child(int sid, int o, int op_ref, int dist, int h,
                                    int parent_gc, int parent_rc, bool force_preferred) {
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
                int nov = full_novelty(child.data(), num_unsat_goals[cid],
                                       num_unsat_relevant[cid]);
                int h_lm = lm ? lm->compute(accepted_of(cid), child.data()) : 0;
                insert(cid, h, h_lm, dist, num_unsat_goals[cid], nov, false);
            }
        }
        return true;
    }
    add_node(cid);
    parent[cid] = sid;
    parent_op[cid] = op_ref;
    g_value[cid] = dist;
    int gc = ctx.count_unsatisfied_goals(child.data());
    int rc = ctx.count_unsatisfied_relevant(child.data());
    num_unsat_goals[cid] = gc;
    num_unsat_relevant[cid] = rc;
    if (gc == 0) {
        goal_state = cid;
        return false;
    }
    int h_lm = 0;
    if (lm) {
        memcpy(accepted_of(cid), child_accepted.data(), sizeof(uint64_t) * lm_words);
        h_lm = lm->compute(accepted_of(cid), child.data());
    }
    int nov = 0;
    if (config.novelty && lazy_novelty) {
        nov = parent_novelty;
    }
    else if (config.novelty) {
        // Novelty partition: (#g, #r), or the landmark count (as in NOLAN).
        int pg = gc, pr = rc, parent_pg = parent_gc, parent_pr = parent_rc;
        if (novelty_by_lm) {
            pg = parent_pg = 0;
            pr = h_lm;
            parent_pr = parent_h_lm;
        }
        if (!ctx.full_novelty_check && pg == parent_pg && pr == parent_pr)
            nov = effects_novelty(child.data(), pg, pr);
        else
            nov = full_novelty(child.data(), pg, pr);
        if (config.prune_novelty > 0 && nov > config.prune_novelty) {
            ++pruned_by_novelty;
            return true;
        }
    }
    bool preferred = force_preferred;
    if (helpful_actions && !preferred) {
        // Helpful if some operator of the transition is in the relaxed plan.
        if (op_ref >= 0) {
            preferred = in_relaxed_plan[o];
        }
        else {
            for (int x : chain) {
                if (in_relaxed_plan[x]) {
                    preferred = true;
                    break;
                }
            }
        }
    }
    for (int f : added) {
        if (marks[f] & (helpful_actions ? 4 : 5)) {
            preferred = true;
            break;
        }
    }
    if (!preferred) {
        for (int f : removed) {
            if (marks[f] & (helpful_actions ? 8 : 10)) {
                preferred = true;
                break;
            }
        }
    }
    insert(cid, h, h_lm, dist, gc, nov, preferred);
    return true;
}

/*
  Relaxed plan lookahead (Vidal 2004): apply the operators of the relaxed
  plan of the expanded state, in the order of their h-add layers, as long
  as some of them is applicable; the resulting state is added as a
  preferred successor.
*/
void GroundSearchRun::relaxed_plan_lookahead(int sid, int g, int h, int parent_gc,
                                             int parent_rc) {
    const vector<int> &rp = heuristic.get_relaxed_plan();
    const vector<int> &levels = heuristic.get_relaxed_plan_levels();
    if (rp.size() < 2)
        return;
    lookahead_order.resize(rp.size());
    for (size_t i = 0; i < rp.size(); ++i)
        lookahead_order[i] = i;
    stable_sort(lookahead_order.begin(), lookahead_order.end(),
                [&](int a, int b) { return levels[a] < levels[b]; });
    vector<char> used(rp.size(), 0);
    memcpy(child.data(), buffer.data(), sizeof(uint64_t) * num_words);
    chain.clear();
    int dist = g;
    GroundSuccessorGenerator &generator = ctx.generator;
    while (true) {
        int found = -1;
        for (int i : lookahead_order) {
            if (!used[i] && generator.is_applicable(child.data(), rp[i])) {
                found = i;
                break;
            }
        }
        if (found < 0)
            break;
        used[found] = 1;
        chain_added.clear();
        chain_removed.clear();
        generator.apply(child.data(), rp[found], chain_child.data(), chain_added, chain_removed);
        child.swap(chain_child);
        chain.push_back(rp[found]);
        dist += task.operators[rp[found]].cost;
        if (ctx.count_unsatisfied_goals(child.data()) == 0)
            break;
    }
    if (chain.size() < 2)
        return;
    if (ctx.compress_chains) {
        // Continue with forced operators (e.g., to finish a sequence of
        // operators that only makes sense as a whole).
        cycle_ref = child;
        int power = 1, length = 0;
        for (int steps = 0; steps < 10000; ++steps) {
            if (ctx.count_unsatisfied_goals(child.data()) == 0)
                break;
            generator.get_applicable(child.data(), chain_facts, chain_ops, 2);
            if (chain_ops.empty())
                return;
            if (chain_ops.size() != 1)
                break;
            chain_added.clear();
            chain_removed.clear();
            generator.apply(child.data(), chain_ops[0], chain_child.data(), chain_added,
                            chain_removed);
            child.swap(chain_child);
            chain.push_back(chain_ops[0]);
            dist += task.operators[chain_ops[0]].cost;
            if (child == cycle_ref)
                return;  // Forced cycle.
            if (++length == power) {
                cycle_ref = child;
                power *= 2;
                length = 0;
            }
        }
    }
    ++lookahead_states;
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
    if (lm)
        lm->update_accepted(parent_accepted.data(), child.data(), child_accepted.data());
    process_child(sid, chain[0], -int(chains.size()) - 2, dist, h, parent_gc, parent_rc,
                  !lookahead_not_preferred);
}

GroundedSearch::GroundedSearch(const Task &task, shared_ptr<GroundTask> gt,
                               const string &heuristic_name, bool full_novelty_check)
    : task(task), ground_task(std::move(gt)), full_novelty_check(full_novelty_check),
      compress_chains(!getenv("PWL_NO_CHAINS")), generator(*ground_task) {
    GroundedHeuristic::Type type = GroundedHeuristic::FF;
    if (heuristic_name == "add")
        type = GroundedHeuristic::ADD;
    else if (heuristic_name == "hmax")
        type = GroundedHeuristic::MAX;
    heuristic = make_unique<GroundedHeuristic>(task, ground_task, type);
    generator.restrict_operators(heuristic->get_relevant_actions());
    num_words = (ground_task->num_facts() + 63) / 64;
    if (num_words == 0)
        num_words = 1;
    goal_pos = ground_task->goal;
    goal_neg = ground_task->negated_goal;
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

int GroundedSearch::count_unsatisfied_relevant(const uint64_t *bits) const {
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
}

void GroundedSearch::print_plan_of_ops(const vector<int> &ops) const {
    vector<LiftedOperatorId> plan;
    for (int o : ops) {
        const GroundOperator &op = ground_task->operators[o];
        vector<int> instantiation = op.instantiation;
        plan.emplace_back(op.schema, std::move(instantiation), unordered_map<int, int>());
    }
    print_plan(plan, task);
}

// Current size of the address space of the process in bytes (0 if
// unknown).
static size_t current_address_space() {
    FILE *f = fopen("/proc/self/statm", "r");
    if (!f)
        return 0;
    unsigned long pages = 0;
    if (fscanf(f, "%lu", &pages) != 1)
        pages = 0;
    fclose(f);
    return size_t(pages) * size_t(sysconf(_SC_PAGESIZE));
}

utils::ExitCode GroundedSearch::search(const vector<GroundSearchConfig> &configs) {
    cout << "Starting grounded search" << endl;
    clock_t timer_start = clock();

    if (ground_task->goal_unreachable) {
        cout << "Goal is not relaxed reachable." << endl;
        print_no_solution_found(timer_start);
        return utils::ExitCode::SEARCH_UNSOLVABLE;
    }

    initial_bits.assign(num_words, 0);
    for (int f : ground_task->initial_facts)
        set_bit(initial_bits.data(), f);
    generator.evaluate_axioms(initial_bits.data());

    vector<int> state_facts;
    collect_true_facts(initial_bits.data(), num_words, state_facts);
    initial_h = heuristic->compute_for_facts(state_facts, false);
    if (getenv("PWL_BENCH_H")) {
        // Micro-benchmark: evaluate the initial state repeatedly.
        int n = atoi(getenv("PWL_BENCH_H"));
        clock_t start = clock();
        for (int i = 0; i < n; ++i)
            heuristic->compute_for_facts(state_facts, false);
        cout << "Heuristic benchmark: " << n << " evaluations in "
             << double(clock() - start) / CLOCKS_PER_SEC << "s" << endl;
    }
    cout << "Initial heuristic value: " << initial_h << endl;
    cout << "Initial h-add value: " << heuristic->get_last_hadd() << endl;
    vector<int> relevant = heuristic->get_useful_facts();
    sort(relevant.begin(), relevant.end());
    cout << "Total number of relevant atoms: " << relevant.size() << endl;
    const int num_base_facts = heuristic->get_num_base_facts();
    for (int f : relevant) {
        if (f < num_base_facts)
            relevant_pos.push_back(f);
        else
            relevant_neg.push_back(heuristic->get_complement_base(f));
    }

    if (count_unsatisfied_goals(initial_bits.data()) == 0) {
        print_goal_time(timer_start);
        print_plan_of_ops({});
        return utils::ExitCode::SUCCESS;
    }
    if (initial_h == UNSOLVABLE_STATE) {
        print_no_solution_found(timer_start);
        return utils::ExitCode::SEARCH_UNSOLVABLE;
    }

    bool need_landmarks = false;
    for (const GroundSearchConfig &c : configs)
        need_landmarks = need_landmarks || c.lm;
    if (need_landmarks) {
        vector<int> init_relaxed = state_facts;
        heuristic->add_complement_facts(state_facts, init_relaxed);
        landmarks = make_unique<LandmarkCountHeuristic>(*heuristic, init_relaxed, 200000000LL);
        if (!landmarks->is_ok())
            landmarks.reset();
        else
            cout << "Initial landmark count: " << landmarks->get_num_landmarks() << endl;
    }

    if (getenv("PWL_TRACE_PLAN")) {
        // Debugging: evaluate the heuristics along the plan in the given
        // file (one "(name args)" per line).
        unordered_map<string, int> op_of_name;
        for (size_t o = 0; o < ground_task->operators.size(); ++o) {
            const GroundOperator &op = ground_task->operators[o];
            const ActionSchema &a = task.get_action_schema_by_index(op.schema);
            string name = "(" + a.get_name();
            for (size_t i = 0; i < min(op.instantiation.size(),
                                       size_t(a.get_num_external_parameters())); ++i)
                name += " " + task.objects[op.instantiation[i]].get_name();
            op_of_name[name + ")"] = o;
        }
        ifstream in(getenv("PWL_TRACE_PLAN"));
        string line;
        vector<uint64_t> bits = initial_bits, next(num_words);
        vector<uint64_t> acc, next_acc;
        if (landmarks) {
            acc.assign(landmarks->get_num_words(), 0);
            next_acc.assign(landmarks->get_num_words(), 0);
            landmarks->initial_accepted(bits.data(), acc.data());
        }
        vector<int> added, removed, facts;
        int step = 0;
        while (true) {
            collect_true_facts(bits.data(), num_words, facts);
            int h = heuristic->compute_for_facts(facts, false);
            cout << "Trace step " << step << ": h=" << h
                 << " h_add=" << heuristic->get_last_hadd()
                 << " h_lm=" << (landmarks ? landmarks->compute(acc.data(), bits.data()) : -1)
                 << " goals=" << count_unsatisfied_goals(bits.data()) << endl;
            if (!getline(in, line))
                break;
            transform(line.begin(), line.end(), line.begin(), ::tolower);
            while (!line.empty() && (line.back() == ' ' || line.back() == '\r'))
                line.pop_back();
            if (line.empty() || line[0] != '(')
                break;
            auto it = op_of_name.find(line);
            if (it == op_of_name.end()) {
                cout << "Trace: unknown operator " << line << endl;
                break;
            }
            generator.apply(bits.data(), it->second, next.data(), added, removed);
            if (landmarks) {
                landmarks->update_accepted(acc.data(), next.data(), next_acc.data());
                acc.swap(next_acc);
            }
            bits.swap(next);
            ++step;
        }
    }

    // Staged runs (pruned or limited) run first, one after the other.
    for (const GroundSearchConfig &c : configs) {
        if (c.prune_novelty <= 0 && c.max_expansions < 0)
            continue;
        GroundSearchRun run(*this, c);
        clock_t start = clock();
        GroundSearchRun::Status status = run.step(c.max_seconds);
        run.time_used = double(clock() - start) / CLOCKS_PER_SEC;
        cout << "Staged search run " << run.get_name() << ": " << run.expanded
             << " expanded, " << run.time_used << "s" << endl;
        staged_expanded += run.expanded;
        staged_evaluated += run.evaluated;
        staged_generated += run.generated;
        if (status == GroundSearchRun::SOLVED) {
            print_goal_time(timer_start);
            cout << "Solved by search run " << run.get_name() << endl;
            vector<int> ops;
            run.get_plan(ops);
            print_plan_of_ops(ops);
            return utils::ExitCode::SUCCESS;
        }
    }

    for (GroundSearchConfig c : configs) {
        if (c.prune_novelty > 0 || c.max_expansions >= 0)
            continue;
        if (configs.size() > 1 && c.width > 1 &&
            ground_task->num_facts() > GroundNovelty::MAX_FACTS_FOR_WIDTH_2) {
            // Width 2 is infeasible: diversify with a random successor order.
            c.width = 1;
            c.shuffle = true;
            c.seed = 1;
            c.name += "-w1-shuffled";
        }
        runs.push_back(make_unique<GroundSearchRun>(*this, c));
    }

    // If the address space of the process grows beyond this, the largest
    // run is dropped (the address space is limited to 4 GiB).
    const size_t memory_budget = size_t(3300) * 1024 * 1024;
    // 1: active, 0: dropped, 2: not started yet (start_time not reached).
    vector<char> active(runs.size(), 1);
    int num_active = 0;
    for (size_t i = 0; i < runs.size(); ++i) {
        if (runs[i]->get_start_time() > 0)
            active[i] = 2;
        else
            ++num_active;
    }
    const double time_slice = runs.size() > 1 ? 0.05 : 1e9;
    // For benchmarking: stop after this many expansions.
    long long expansion_limit = getenv("PWL_EXPANSION_LIMIT")
        ? atoll(getenv("PWL_EXPANSION_LIMIT")) : -1;
    while (num_active > 0) {
        if (expansion_limit >= 0) {
            long long total = 0;
            for (const auto &r : runs)
                total += r ? r->expanded : 0;
            if (total >= expansion_limit) {
                cout << "Expansion limit reached after "
                     << double(clock() - timer_start) / CLOCKS_PER_SEC << "s" << endl;
                return utils::ExitCode::SEARCH_UNSOLVED_INCOMPLETE;
            }
        }
        double elapsed = double(clock() - timer_start) / CLOCKS_PER_SEC;
        for (size_t i = 0; i < runs.size(); ++i) {
            if (active[i] == 2 && elapsed >= runs[i]->get_start_time()) {
                // Start with the least time used so far, to share the time
                // equally from now on.
                double least = 1e18;
                for (size_t j = 0; j < runs.size(); ++j) {
                    if (active[j] == 1)
                        least = min(least, runs[j]->time_used);
                }
                runs[i]->time_used = least < 1e18 ? least : 0;
                active[i] = 1;
                ++num_active;
                cout << "Starting search run " << runs[i]->get_name() << " after " << elapsed
                     << "s." << endl;
            }
        }
        int best = -1;
        for (size_t i = 0; i < runs.size(); ++i) {
            if (active[i] == 1 && (best < 0 || runs[i]->time_used < runs[best]->time_used))
                best = i;
        }
        GroundSearchRun &run = *runs[best];
        clock_t start = clock();
        GroundSearchRun::Status status = run.step(time_slice);
        run.time_used += double(clock() - start) / CLOCKS_PER_SEC;
        if (status == GroundSearchRun::SOLVED) {
            print_goal_time(timer_start);
            cout << "Solved by search run " << run.get_name() << endl;
            vector<int> ops;
            run.get_plan(ops);
            print_plan_of_ops(ops);
            return utils::ExitCode::SUCCESS;
        }
        if (status == GroundSearchRun::EXHAUSTED) {
            cout << "Search run " << run.get_name() << " exhausted its search space." << endl;
            // Searches are complete (only dead ends are pruned).
            print_no_solution_found(timer_start);
            return utils::ExitCode::SEARCH_UNSOLVABLE;
        }
        if (num_active > 1) {
            int largest = -1;
            for (size_t i = 0; i < runs.size(); ++i) {
                if (active[i] == 1 &&
                    (largest < 0 || runs[i]->memory_usage() > runs[largest]->memory_usage()))
                    largest = i;
            }
            if (current_address_space() > memory_budget) {
                cout << "Dropping search run " << runs[largest]->get_name()
                     << " to save memory (" << runs[largest]->expanded << " expanded)."
                     << endl;
                active[largest] = 0;
                --num_active;
                runs[largest].reset();
                malloc_trim(0);
            }
        }
    }
    print_no_solution_found(timer_start);
    return utils::ExitCode::SEARCH_UNSOLVABLE;
}

void GroundedSearch::print_statistics() const {
    long long expanded = staged_expanded, evaluated = staged_evaluated,
        generated = staged_generated, registered = 0;
    for (const auto &run : runs) {
        if (!run)
            continue;
        expanded += run->expanded;
        evaluated += run->evaluated;
        generated += run->generated;
        registered += run->num_registered();
        cout << "Search run " << run->get_name() << ": " << run->expanded << " expanded, "
             << run->num_registered() << " registered, " << run->time_used << "s, "
             << run->compressed_chains << " compressed chains (average length "
             << (run->compressed_chains ? double(run->chain_length_sum) / run->compressed_chains
                                        : 0.0)
             << "), " << run->pruned_dead_ends << " pruned dead ends" << endl;
    }
    cout << "Expanded " << expanded << " state(s)." << endl;
    cout << "Evaluated " << evaluated << " state(s)." << endl;
    cout << "Generated " << generated << " state(s)." << endl;
    cout << "Registered " << registered << " state(s)." << endl;
}

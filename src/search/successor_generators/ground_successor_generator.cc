#include "ground_successor_generator.h"

#include "../grounding/ground_task.h"
#include "../states/ground_state.h"

#include <algorithm>
#include <cstring>
#include <iterator>

using namespace std;
using namespace ground_state;

namespace {
// Concatenate the lists (compressed sparse rows): list i is
// data[start[i] .. start[i + 1]).
void build_csr(const vector<const vector<int> *> &lists, vector<int> &start, vector<int> &data)
{
    start.clear();
    data.clear();
    for (const vector<int> *l : lists) {
        start.push_back(data.size());
        data.insert(data.end(), l->begin(), l->end());
    }
    start.push_back(data.size());
}
}  // namespace

GroundSuccessorGenerator::GroundSuccessorGenerator(const GroundTask &task)
    : task(task), num_words(get_num_words(task.num_facts()))
{
    const auto &ops = task.operators;
    vector<const vector<int> *> pre_lists, negated_pre_lists;
    for (const GroundOperator &op : ops) {
        pre_lists.push_back(&op.pre);
        negated_pre_lists.push_back(&op.negated_pre);
    }
    build_csr(pre_lists, pre_start, pre);
    build_csr(negated_pre_lists, negated_pre_start, negated_pre);

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
            ops_without_pre.push_back(o);
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
    build_watch_index();

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
        axiom_head.push_back(ax.head);
        axiom_stratum.push_back(ax.stratum);
        set_bit(derived_mask.data(), ax.head);
    }
    stratum_start.push_back(order.size());
    build_csr(body_lists, axiom_body_start, axiom_body);
    build_csr(neg_body_lists, axiom_negated_body_start, axiom_negated_body);
    vector<vector<int>> axioms_by_body_fact_lists(num_facts);
    for (size_t k = 0; k < order.size(); ++k) {
        for (int i = axiom_body_start[k]; i < axiom_body_start[k + 1]; ++i)
            axioms_by_body_fact_lists[axiom_body[i]].push_back(k);
    }
    vector<const vector<int> *> axioms_by_body_fact_ptrs;
    for (const auto &l : axioms_by_body_fact_lists)
        axioms_by_body_fact_ptrs.push_back(&l);
    build_csr(axioms_by_body_fact_ptrs, axioms_by_body_fact_start, axioms_by_body_fact);
    axiom_num_unsatisfied.assign(order.size(), 0);
}

/*
  Second level of the watch index: the operators watched by a fact f are
  grouped by a second precondition g (the one occurring in the fewest of
  these operators), so that only groups whose fact g is true are scanned.
*/
void GroundSuccessorGenerator::build_watch_index()
{
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

    // Remaining literals of the grouped operators, contiguous in group order
    // (without the watched fact and the group's fact; negated preconditions
    // f encoded as -1 - f), so that scanning a group reads memory in order.
    group_literal_start.clear();
    group_literals.clear();
    for (int f = 0; f < num_facts; ++f) {
        for (int i = group_start[f]; i < group_start[f + 1]; ++i) {
            for (int k = group_op_start[i]; k < group_op_start[i + 1]; ++k) {
                int o = group_ops[k];
                group_literal_start.push_back(group_literals.size());
                for (int j = pre_start[o]; j < pre_start[o + 1]; ++j) {
                    if (pre[j] != f && pre[j] != group_fact[i])
                        group_literals.push_back(pre[j]);
                }
                for (int j = negated_pre_start[o]; j < negated_pre_start[o + 1]; ++j)
                    group_literals.push_back(-1 - negated_pre[j]);
            }
        }
    }
    group_literal_start.push_back(group_literals.size());

    // Blocker of each group: a negated precondition shared by all its
    // operators (the group is skipped if it is true), or -1.
    group_blocker.assign(group_fact.size(), -1);
    vector<int> common, next_common;
    for (size_t i = 0; i < group_fact.size(); ++i) {
        int first = group_op_start[i], last = group_op_start[i + 1];
        common.assign(negated_pre.begin() + negated_pre_start[group_ops[first]],
                      negated_pre.begin() + negated_pre_start[group_ops[first] + 1]);
        sort(common.begin(), common.end());
        for (int k = first + 1; k < last && !common.empty(); ++k) {
            int o = group_ops[k];
            vector<int> n(negated_pre.begin() + negated_pre_start[o],
                          negated_pre.begin() + negated_pre_start[o + 1]);
            sort(n.begin(), n.end());
            next_common.clear();
            set_intersection(
                common.begin(), common.end(), n.begin(), n.end(), back_inserter(next_common));
            common.swap(next_common);
        }
        if (!common.empty())
            group_blocker[i] = common[0];
    }

    /*
      Facts watching many groups get a bitset of their groups' facts: ANDed
      with the state, it yields the groups to scan; a group's index is the
      rank of its fact in the bitset (groups are sorted by fact).
    */
    group_mask_start.assign(num_facts, -1);
    group_masks.clear();
    const size_t max_mask_words = size_t(16) * 1024 * 1024;
    for (int f = 0; f < num_facts; ++f) {
        int first = group_start[f], last = group_start[f + 1];
        if (first < last && group_fact[first] < 0)
            ++first;
        if (last - first <= 2 * num_words || group_masks.size() + num_words > max_mask_words)
            continue;
        group_mask_start[f] = group_masks.size();
        group_masks.resize(group_masks.size() + num_words, 0);
        for (int i = first; i < last; ++i)
            set_bit(group_masks.data() + group_mask_start[f], group_fact[i]);
    }
}

void GroundSuccessorGenerator::restrict_operators(const vector<char> &allowed)
{
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
    build_watch_index();
    vector<int> new_ops_without_pre;
    for (int o : ops_without_pre) {
        if (allowed[o])
            new_ops_without_pre.push_back(o);
    }
    ops_without_pre.swap(new_ops_without_pre);
}

bool GroundSuccessorGenerator::is_applicable(const uint64_t *bits, int o) const
{
    for (int i = pre_start[o]; i < pre_start[o + 1]; ++i) {
        if (!test_bit(bits, pre[i]))
            return false;
    }
    for (int i = negated_pre_start[o]; i < negated_pre_start[o + 1]; ++i) {
        if (test_bit(bits, negated_pre[i]))
            return false;
    }
    return true;
}

void GroundSuccessorGenerator::get_applicable(const uint64_t *bits,
                                              vector<int> &ops,
                                              size_t max_ops) const
{
    ops.clear();
    auto check = [&](int o) {
        for (int i = pre_start[o]; i < pre_start[o + 1]; ++i) {
            if (!test_bit(bits, pre[i]))
                return false;
        }
        for (int i = negated_pre_start[o]; i < negated_pre_start[o + 1]; ++i) {
            if (test_bit(bits, negated_pre[i]))
                return false;
        }
        return true;
    };
    for (int o : ops_without_pre) {
        if (check(o)) {
            ops.push_back(o);
            if (ops.size() >= max_ops)
                return;
        }
    }
    for (int w = 0; w < num_words; ++w) {
        uint64_t word = bits[w] & watch_mask[w];
        while (word) {
            int f = w * 64 + __builtin_ctzll(word);
            word &= word - 1;
            auto scan_group = [&](int i) {
                if (group_blocker[i] >= 0 && test_bit(bits, group_blocker[i]))
                    return;
                for (int k = group_op_start[i]; k < group_op_start[i + 1]; ++k) {
                    bool applicable = true;
                    for (int j = group_literal_start[k]; j < group_literal_start[k + 1]; ++j) {
                        int lit = group_literals[j];
                        if (lit >= 0 ? !test_bit(bits, lit) : test_bit(bits, -1 - lit)) {
                            applicable = false;
                            break;
                        }
                    }
                    if (applicable) {
                        ops.push_back(group_ops[k]);
                        if (ops.size() >= max_ops)
                            return;
                    }
                }
            };
            if (ops.size() >= max_ops)
                return;
            if (group_mask_start[f] >= 0) {
                int i = group_start[f];
                if (i < group_start[f + 1] && group_fact[i] < 0) {
                    scan_group(i++);
                    if (ops.size() >= max_ops)
                        return;
                }
                const uint64_t *mask = group_masks.data() + group_mask_start[f];
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
                        if (ops.size() >= max_ops)
                            return;
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
                if (ops.size() >= max_ops)
                    return;
            }
        }
    }
}

void GroundSuccessorGenerator::evaluate_axioms(uint64_t *bits)
{
    if (!has_axioms)
        return;
    for (int w = 0; w < num_words; ++w)
        bits[w] &= ~derived_mask[w];
    for (size_t s = 0; s + 1 < stratum_start.size(); ++s) {
        int begin = stratum_start[s], end = stratum_start[s + 1];
        int stratum = axiom_stratum[begin];
        derived_queue.clear();
        for (int k = begin; k < end; ++k) {
            bool blocked = false;
            for (int i = axiom_negated_body_start[k]; i < axiom_negated_body_start[k + 1]; ++i) {
                if (test_bit(bits, axiom_negated_body[i])) {
                    blocked = true;
                    break;
                }
            }
            if (blocked) {
                axiom_num_unsatisfied[k] = -1;
                continue;
            }
            int count = 0;
            for (int i = axiom_body_start[k]; i < axiom_body_start[k + 1]; ++i) {
                if (!test_bit(bits, axiom_body[i]))
                    ++count;
            }
            axiom_num_unsatisfied[k] = count;
        }
        auto fire = [&](int k) {
            int h = axiom_head[k];
            if (!test_bit(bits, h)) {
                set_bit(bits, h);
                derived_queue.push_back(h);
            }
        };
        for (int k = begin; k < end; ++k) {
            if (axiom_num_unsatisfied[k] == 0)
                fire(k);
        }
        for (size_t q = 0; q < derived_queue.size(); ++q) {
            int h = derived_queue[q];
            for (int i = axioms_by_body_fact_start[h]; i < axioms_by_body_fact_start[h + 1]; ++i) {
                int k = axioms_by_body_fact[i];
                if (axiom_stratum[k] != stratum || axiom_num_unsatisfied[k] <= 0)
                    continue;
                if (--axiom_num_unsatisfied[k] == 0)
                    fire(k);
            }
        }
    }
}

void GroundSuccessorGenerator::apply(
    const uint64_t *parent_bits, int o, uint64_t *child, vector<int> &added, vector<int> &removed)
{
    memcpy(child, parent_bits, sizeof(uint64_t) * num_words);
    const GroundOperator &op = task.operators[o];
    fired_effects.clear();
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
            fired_effects.push_back(c);
    }
    for (int f : op.del)
        reset_bit(child, f);
    for (int c : fired_effects) {
        for (int f : task.conditional_effects[c].del)
            reset_bit(child, f);
    }
    for (int f : op.add)
        set_bit(child, f);
    for (int c : fired_effects) {
        for (int f : task.conditional_effects[c].add)
            set_bit(child, f);
    }
    evaluate_axioms(child);
    get_changes(parent_bits, child, num_words, added, removed);
}

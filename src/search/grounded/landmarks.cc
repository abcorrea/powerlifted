#include "landmarks.h"

#include "../heuristics/grounded_heuristic.h"

#include <algorithm>
#include <deque>
#include <iostream>
#include <iterator>

using namespace std;

namespace {
inline bool test_bit(const uint64_t *b, int i) {
    return (b[i >> 6] >> (i & 63)) & 1ULL;
}

inline void set_bit(uint64_t *b, int i) {
    b[i >> 6] |= 1ULL << (i & 63);
}
}  // namespace

LandmarkCountHeuristic::LandmarkCountHeuristic(const GroundedHeuristic &rel,
                                               const vector<int> &initial_facts,
                                               long long max_work)
    : num_base_facts(rel.get_num_base_facts()), num_words(1), ok(false) {
    const int num_facts = rel.get_num_facts();
    const int num_ops = rel.get_num_relaxed_ops();
    const auto &pre_start = rel.get_op_pre_start();
    const auto &pre = rel.get_op_pre();
    const auto &eff_start = rel.get_op_eff_start();
    const auto &eff = rel.get_op_eff();
    const auto &pre_of_start = rel.get_pre_of_start();
    const auto &pre_of = rel.get_pre_of();

    vector<vector<int>> label(num_facts);
    vector<char> reached(num_facts, 0);
    vector<int> pending(num_ops);
    for (int r = 0; r < num_ops; ++r)
        pending[r] = pre_start[r + 1] - pre_start[r];
    vector<char> queued(num_ops, 0);
    deque<int> queue;
    long long work = 0;

    auto enqueue = [&](int r) {
        if (!queued[r]) {
            queued[r] = 1;
            queue.push_back(r);
        }
    };
    auto reach = [&](int f) {
        reached[f] = 1;
        for (int k = pre_of_start[f]; k < pre_of_start[f + 1]; ++k) {
            int r = pre_of[k];
            if (--pending[r] == 0)
                enqueue(r);
        }
    };
    for (int f : initial_facts) {
        if (!reached[f]) {
            label[f] = {f};
            reach(f);
        }
    }
    for (int r : rel.get_no_pre_ops())
        enqueue(r);

    vector<int> union_label, candidate, intersection;
    while (!queue.empty()) {
        int r = queue.front();
        queue.pop_front();
        queued[r] = 0;
        union_label.clear();
        for (int i = pre_start[r]; i < pre_start[r + 1]; ++i) {
            const vector<int> &l = label[pre[i]];
            union_label.insert(union_label.end(), l.begin(), l.end());
        }
        sort(union_label.begin(), union_label.end());
        union_label.erase(unique(union_label.begin(), union_label.end()), union_label.end());
        work += union_label.size() + 1;
        for (int i = eff_start[r]; i < eff_start[r + 1]; ++i) {
            int e = eff[i];
            candidate = union_label;
            auto pos = lower_bound(candidate.begin(), candidate.end(), e);
            if (pos == candidate.end() || *pos != e)
                candidate.insert(pos, e);
            if (!reached[e]) {
                label[e] = candidate;
                reach(e);
                continue;
            }
            intersection.clear();
            set_intersection(label[e].begin(), label[e].end(), candidate.begin(),
                             candidate.end(), back_inserter(intersection));
            work += label[e].size();
            if (intersection.size() < label[e].size()) {
                label[e].swap(intersection);
                for (int k = pre_of_start[e]; k < pre_of_start[e + 1]; ++k) {
                    int r2 = pre_of[k];
                    if (pending[r2] == 0)
                        enqueue(r2);
                }
            }
        }
        if (work > max_work) {
            cout << "Landmark generation aborted (too much work)." << endl;
            return;
        }
    }

    vector<int> landmarks;
    for (int g : rel.get_goal()) {
        if (!reached[g]) {
            cout << "Landmark generation: goal not relaxed reachable." << endl;
            return;
        }
        landmarks.insert(landmarks.end(), label[g].begin(), label[g].end());
    }
    sort(landmarks.begin(), landmarks.end());
    landmarks.erase(unique(landmarks.begin(), landmarks.end()), landmarks.end());

    fact_to_lm.assign(num_facts, -1);
    for (size_t i = 0; i < landmarks.size(); ++i)
        fact_to_lm[landmarks[i]] = i;
    int num_lms = landmarks.size();
    lm_fact = landmarks;
    lm_base.resize(num_lms);
    lm_is_complement.resize(num_lms);
    lm_is_goal.assign(num_lms, 0);
    for (int i = 0; i < num_lms; ++i) {
        int f = landmarks[i];
        if (f < num_base_facts) {
            lm_base[i] = f;
            lm_is_complement[i] = 0;
        }
        else {
            lm_base[i] = rel.get_complement_base(f);
            lm_is_complement[i] = 1;
        }
    }
    for (int g : rel.get_goal())
        lm_is_goal[fact_to_lm[g]] = 1;

    predecessors.resize(num_lms);
    for (int i = 0; i < num_lms; ++i) {
        for (int f : label[landmarks[i]]) {
            int j = fact_to_lm[f];
            if (j >= 0 && j != i)
                predecessors[i].push_back(j);
        }
    }

    // Greedy-necessary predecessors: landmarks in the precondition of every
    // (reachable) achiever.
    vector<char> has_gn(num_lms, 0);
    vector<vector<int>> gn(num_lms);
    vector<int> pre_lms;
    for (int r = 0; r < num_ops; ++r) {
        if (pending[r] != 0)
            continue;
        pre_lms.clear();
        for (int i = pre_start[r]; i < pre_start[r + 1]; ++i) {
            int j = fact_to_lm[pre[i]];
            if (j >= 0)
                pre_lms.push_back(j);
        }
        sort(pre_lms.begin(), pre_lms.end());
        for (int i = eff_start[r]; i < eff_start[r + 1]; ++i) {
            int m = fact_to_lm[eff[i]];
            if (m < 0)
                continue;
            if (!has_gn[m]) {
                has_gn[m] = 1;
                gn[m] = pre_lms;
            }
            else {
                intersection.clear();
                set_intersection(gn[m].begin(), gn[m].end(), pre_lms.begin(), pre_lms.end(),
                                 back_inserter(intersection));
                gn[m].swap(intersection);
            }
        }
    }
    // Store the reverse relation: gn_predecessors[l] lists the landmarks m
    // that have l as greedy-necessary predecessor.
    gn_predecessors.resize(num_lms);
    for (int m = 0; m < num_lms; ++m) {
        for (int l : gn[m]) {
            if (l != m)
                gn_predecessors[l].push_back(m);
        }
    }

    num_words = (num_lms + 63) / 64;
    if (num_words == 0)
        num_words = 1;
    ok = true;
    int num_orderings = 0;
    for (const auto &p : predecessors)
        num_orderings += p.size();
    cout << "Landmarks: " << num_lms << " (" << num_orderings << " orderings)" << endl;
}

void LandmarkCountHeuristic::initial_accepted(const uint64_t *bits, uint64_t *accepted) const {
    fill(accepted, accepted + num_words, 0);
    for (size_t lm = 0; lm < lm_fact.size(); ++lm) {
        if (is_true(lm, bits) && predecessors[lm].empty())
            set_bit(accepted, lm);
    }
}

void LandmarkCountHeuristic::update_accepted(const uint64_t *parent_accepted,
                                             const uint64_t *bits,
                                             uint64_t *accepted) const {
    copy(parent_accepted, parent_accepted + num_words, accepted);
    for (size_t lm = 0; lm < lm_fact.size(); ++lm) {
        if (test_bit(parent_accepted, lm) || !is_true(lm, bits))
            continue;
        bool all = true;
        for (int p : predecessors[lm]) {
            if (!test_bit(parent_accepted, p)) {
                all = false;
                break;
            }
        }
        if (all)
            set_bit(accepted, lm);
    }
}

int LandmarkCountHeuristic::compute(const uint64_t *accepted, const uint64_t *bits) const {
    int h = 0;
    for (size_t lm = 0; lm < lm_fact.size(); ++lm) {
        if (!test_bit(accepted, lm)) {
            ++h;
            continue;
        }
        if (is_true(lm, bits))
            continue;
        if (lm_is_goal[lm]) {
            ++h;
            continue;
        }
        for (int m : gn_predecessors[lm]) {
            if (!test_bit(accepted, m)) {
                ++h;
                break;
            }
        }
    }
    return h;
}

void LandmarkCountHeuristic::get_interesting_facts(const uint64_t *accepted,
                                                   const uint64_t *bits,
                                                   vector<int> &facts) const {
    facts.clear();
    for (size_t lm = 0; lm < lm_fact.size(); ++lm) {
        if (!test_bit(accepted, lm)) {
            bool all = true;
            for (int p : predecessors[lm]) {
                if (!test_bit(accepted, p)) {
                    all = false;
                    break;
                }
            }
            if (all)
                facts.push_back(lm_fact[lm]);
        }
        else if (!is_true(lm, bits)) {
            bool required = lm_is_goal[lm];
            for (int m : gn_predecessors[lm]) {
                if (required)
                    break;
                if (!test_bit(accepted, m))
                    required = true;
            }
            if (required)
                facts.push_back(lm_fact[lm]);
        }
    }
}

#ifndef SEARCH_GROUNDED_LANDMARKS_H
#define SEARCH_GROUNDED_LANDMARKS_H

#include <cstdint>
#include <vector>

class GroundedHeuristic;

/**
 * @brief Fact landmarks of the delete relaxation and the landmark-count
 * heuristic (in the style of LAMA).
 *
 * Landmarks are computed from the initial state with the label propagation
 * of Zhu and Givan (2003) over the relaxed operators of a GroundedHeuristic
 * (including complement facts): label(f) is the set of facts that every
 * relaxed plan reaching f must achieve. The goal landmarks are the union of
 * the labels of the goal facts, and m' in label(m) induces the natural
 * ordering m' < m. Greedy-necessary predecessors of m are the landmarks in
 * the precondition of every achiever of m.
 *
 * The accepted landmarks of a state are inherited from its parent: a
 * landmark becomes accepted when it is true and all its predecessors were
 * accepted in the parent. h(s) counts the landmarks not accepted plus the
 * accepted ones that are required again (false goals, or false
 * greedy-necessary predecessors of non-accepted landmarks).
 */
class LandmarkCountHeuristic {
    int num_base_facts;
    // For each landmark: its fact (base or complement) and the base fact
    // deciding its truth (a complement is true iff its base is false).
    std::vector<int> lm_fact;
    std::vector<int> lm_base;
    std::vector<char> lm_is_complement;
    std::vector<char> lm_is_goal;
    std::vector<std::vector<int>> predecessors;
    std::vector<std::vector<int>> gn_predecessors;
    //! Landmark index of each (relaxed) fact, or -1.
    std::vector<int> fact_to_lm;
    int num_words;
    bool ok;

    bool is_true(int lm, const uint64_t *bits) const {
        bool base = (bits[lm_base[lm] >> 6] >> (lm_base[lm] & 63)) & 1ULL;
        return lm_is_complement[lm] ? !base : base;
    }

public:
    /**
     * Compute the landmarks for the state whose true facts (base and
     * complement facts) are `initial_facts`. Gives up (is_ok() is false) if
     * the label propagation exceeds `max_work` set operations.
     */
    LandmarkCountHeuristic(const GroundedHeuristic &relaxation,
                           const std::vector<int> &initial_facts, long long max_work);

    bool is_ok() const {
        return ok;
    }

    int get_num_landmarks() const {
        return lm_fact.size();
    }

    //! Number of 64-bit words of an accepted-landmarks bitset.
    int get_num_words() const {
        return num_words;
    }

    //! Accepted landmarks of the initial state.
    void initial_accepted(const uint64_t *bits, uint64_t *accepted) const;

    //! Accepted landmarks of a successor state.
    void update_accepted(const uint64_t *parent_accepted, const uint64_t *bits,
                         uint64_t *accepted) const;

    int compute(const uint64_t *accepted, const uint64_t *bits) const;

    /**
     * Mark (with 1 in `interesting`, indexed by relaxed fact) the landmarks
     * worth achieving next: non-accepted landmarks whose predecessors are
     * all accepted, and landmarks required again.
     */
    void get_interesting_facts(const uint64_t *accepted, const uint64_t *bits,
                               std::vector<int> &facts) const;

    int get_landmark_fact(int lm) const {
        return lm_fact[lm];
    }
};

#endif  // SEARCH_GROUNDED_LANDMARKS_H

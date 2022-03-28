#ifndef SEARCH_NOVELTY_NODE_NOVELTY_H_
#define SEARCH_NOVELTY_NODE_NOVELTY_H_

struct NodeNovelty {
    int unsatisfied_goals;
    int unsatisfied_relevant_atoms;

    NodeNovelty(int g, int r) : unsatisfied_goals(g), unsatisfied_relevant_atoms(r) {}
};

#endif //SEARCH_NOVELTY_NODE_NOVELTY_H_

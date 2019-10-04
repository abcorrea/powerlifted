#ifndef SEARCH_UNION_FIND_H
#define SEARCH_UNION_FIND_H

#include <vector>

/**
 * @brief Implements Union-find data structure without path compression
 */
class UnionFind {
public:
    explicit UnionFind(int n) {
        root.resize(n);
        rank.resize(n);
        for (int i = 0; i < n; i++) {
            root[i] = i;
            rank[i] = 0;
        }
    }

    int find(int x) {
        while (root[x] != x) {
            x = root[x];
        }
        return x;
    }

    void union_func(int x, int y) {
        int x_root = find(x);
        int y_root = find(y);
        if (x_root == y_root)
            return;
        root[y_root] = x_root;
        if (rank[x_root] == rank[y_root]) {
            rank[x_root]++;
        }
    }

    std::vector<int> get_components() {
        std::vector<int> r;
        for (int i = 0; i < root.size(); i++) {
            if (root[i] == i)
                r.push_back(i);
        }
        return r;
    }


private:
    std::vector<int> root;
    std::vector<int> rank;

};

#endif //SEARCH_UNION_FIND_H

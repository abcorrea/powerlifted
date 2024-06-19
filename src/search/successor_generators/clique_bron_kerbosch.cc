#include "clique_bron_kerbosch.h"
#include "clique_help_functions.h"

#include <algorithm>
#include <limits>
#include <vector>

using namespace std;

inline void insert_sorted(vector<uint32_t> &vec, uint32_t item) {
    vec.insert(upper_bound(vec.begin(), vec.end(), item), item);
}

inline void erase_index(vector<uint32_t> &vec, size_t index) {
    vec.erase(vec.begin() + index);
}

inline void erase_sorted(vector<uint32_t> &vec, uint32_t item) {
    auto lb = lower_bound(begin(vec), end(vec), item);
    if ((lb != end(vec)) && (*lb == item)) {
        vec.erase(lb, lb + 1);
    }
}

template <typename P>
void bron_kerbosch_with_pivoting(const vector<uint32_t> adjacency_list[],
                                 const size_t max_clique_size,
                                 const vector<uint32_t> &r_sorted,
                                 const vector<uint32_t> &p_sorted,
                                 const vector<uint32_t> &x_sorted,
                                 vector<vector<uint32_t>> &out_cliques) {
    vector<tuple<vector<uint32_t>, vector<uint32_t>, vector<uint32_t>>> stack;
    stack.push_back(make_tuple(r_sorted, p_sorted, x_sorted));

    while (!stack.empty()) {
        auto &frame = stack.back();
        auto r = get<0>(frame);
        auto p = get<1>(frame);
        auto x = get<2>(frame);
        stack.pop_back();

        if ((r.size() + p.size()) < max_clique_size) {
            // This branch cannot lead to a maximum clique: prune it.
        }
        else if ((p.size() == 0) && (x.size() == 0)) {
            if (r.size() == max_clique_size) {
                // If P and X are both empty then report R as a maximum clique
                out_cliques.push_back(r);
            }
        }
        else if (p.size() > 0) {
            const auto u = P::get_pivot(adjacency_list, p, x);
            const auto &n_u = adjacency_list[u];

            size_t p_i = 0;
            size_t n_u_i = 0;

            while (p_i < p.size()) {
                const auto v = p[p_i];
                const auto w = (n_u_i < n_u.size()) ? n_u[n_u_i] : numeric_limits<uint32_t>::max();

                if (v < w) {
                    const auto &n_v = adjacency_list[v];

                    vector<uint32_t> union_r = r;
                    insert_sorted(union_r, v);

                    vector<uint32_t> intersect_p;
                    set_intersection(n_v.begin(),
                                     n_v.end(),
                                     p.begin(),
                                     p.end(),
                                     back_inserter(intersect_p));

                    vector<uint32_t> intersect_x;
                    set_intersection(n_v.begin(),
                                     n_v.end(),
                                     x.begin(),
                                     x.end(),
                                     back_inserter(intersect_x));

                    stack.push_back(make_tuple(std::move(union_r),
                                               std::move(intersect_p),
                                               std::move(intersect_x)));

                    erase_index(p, p_i);  // This also moves the p_i forward

                    insert_sorted(x, v);
                }
                else if (v > w) {
                    ++n_u_i;
                }
                else {
                    ++p_i;
                    ++n_u_i;
                }
            }
        }
    }
}

template <typename P>
void bron_kerbosch_with_vertex_ordering(const vector<uint32_t> adjacency_list[],
                                        const vector<uint32_t> &vertices,
                                        size_t max_clique_size,
                                        vector<vector<uint32_t>> &out_cliques) {
    vector<uint32_t> degeneracy_ordering(vertices);
    sort(degeneracy_ordering.begin(),
         degeneracy_ordering.end(),
         [&adjacency_list](uint32_t lhs, uint32_t rhs) {
             return adjacency_list[lhs].size() < adjacency_list[rhs].size();
         });

    vector<uint32_t> p(vertices);
    sort(p.begin(), p.end());
    vector<uint32_t> x;

    vector<uint32_t> singleton_v;
    vector<uint32_t> intersect_p;
    vector<uint32_t> intersect_x;

    for (auto vertex_id : degeneracy_ordering) {
        const auto &vertex_neighbors = adjacency_list[vertex_id];

        singleton_v.push_back(vertex_id);

        intersect_p.clear();
        set_intersection(vertex_neighbors.begin(),
                         vertex_neighbors.end(),
                         p.begin(),
                         p.end(),
                         back_inserter(intersect_p));

        intersect_x.clear();
        set_intersection(vertex_neighbors.begin(),
                         vertex_neighbors.end(),
                         x.begin(),
                         x.end(),
                         back_inserter(intersect_x));

        bron_kerbosch_with_pivoting<P>(adjacency_list,
                                       max_clique_size,
                                       singleton_v,
                                       intersect_p,
                                       intersect_x,
                                       out_cliques);

        singleton_v.pop_back();

        erase_sorted(p, vertex_id);
        insert_sorted(x, vertex_id);
    }
}

struct FirstPivot {
    static uint32_t get_pivot(const vector<uint32_t> adjacency_list[],
                              const vector<uint32_t> &p,
                              const vector<uint32_t> &x) {
        return (x.size() > 0) ? x.front() : p.front();
    }
};


struct MaxNeighborhoodPivot {
    inline static uint32_t get_pivot(const vector<uint32_t> adjacency_list[],
                                     const vector<uint32_t> &p,
                                     const vector<uint32_t> &x) {
        // Find the vertex v that maximizes |n(v)|.

        bool initialized = false;
        uint32_t best_vertex = p[0];
        size_t best_score = 0;

        for (const auto v : p) {
            const auto score = adjacency_list[v].size();

            if (!initialized || (score >= best_score)) {
                initialized = true;
                best_vertex = v;
                best_score = score;
            }
        }

        for (const auto v : x) {
            const auto score = adjacency_list[v].size();

            if (!initialized || (score >= best_score)) {
                initialized = true;
                best_vertex = v;
                best_score = score;
            }
        }

        return best_vertex;
    }
};

struct MinDifferencePivot {
    inline static int32_t difference_size(const vector<uint32_t> &p, const vector<uint32_t> &n) {
        // The score is |p - n|

        if ((p.back() < n.front()) || (p.front() > n.back())) {
            return p.size();
        }
        else {
            int32_t score = 0;

            size_t p_size = p.size();
            size_t n_size = n.size();

            size_t p_i = 0;
            size_t n_i = 0;

            while (p_i < p_size) {
                const auto u = p[p_i];

                if (n_i < n_size) {
                    const auto v = n[n_i];

                    if (u < v) {
                        ++score;
                        ++p_i;
                    }
                    else if (u > v) {
                        ++n_i;
                    }
                    else {
                        ++p_i;
                        ++n_i;
                    }
                }
                else {
                    score += p_size - p_i;
                    break;
                }
            }

            return score;
        }
    }

    inline static uint32_t get_pivot(const vector<uint32_t> adjacency_list[],
                                     const vector<uint32_t> &p,
                                     const vector<uint32_t> &x) {
        // Find the vertex v that minimizes |p - n(v)|.

        uint32_t best_vertex = numeric_limits<uint32_t>::max();
        int32_t best_score = numeric_limits<int32_t>::max();

        for (const auto v : p) {
            const auto score = difference_size(p, adjacency_list[v]);

            if (score <= best_score) {
                best_vertex = v;
                best_score = score;
            }
        }

        for (const auto v : x) {
            const auto score = difference_size(p, adjacency_list[v]);

            if (score <= best_score) {
                best_vertex = v;
                best_score = score;
            }
        }

        return best_vertex;
    }
};

void bron_kerbosch_first_pivot(const vector<uint32_t> adjacency_list[],
                               const vector<uint32_t> &vertices,
                               size_t max_clique_size,
                               vector<vector<uint32_t>> &out_cliques) {
    bron_kerbosch_with_vertex_ordering<FirstPivot>(adjacency_list,
                                                   vertices,
                                                   max_clique_size,
                                                   out_cliques);
}

void bron_kerbosch_max_neighborhood_pivot(const vector<uint32_t> adjacency_list[],
                                          const vector<uint32_t> &vertices,
                                          size_t max_clique_size,
                                          vector<vector<uint32_t>> &out_cliques) {
    bron_kerbosch_with_vertex_ordering<MaxNeighborhoodPivot>(adjacency_list,
                                                             vertices,
                                                             max_clique_size,
                                                             out_cliques);
}

void bron_kerbosch_min_difference_pivot(const vector<uint32_t> adjacency_list[],
                                        const vector<uint32_t> &vertices,
                                        size_t max_clique_size,
                                        vector<vector<uint32_t>> &out_cliques) {
    bron_kerbosch_with_vertex_ordering<MinDifferencePivot>(adjacency_list,
                                                           vertices,
                                                           max_clique_size,
                                                           out_cliques);
}

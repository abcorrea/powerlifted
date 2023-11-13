#include <cstdint>
#include <vector>

void bron_kerbosch_first_pivot(const std::vector<uint32_t> adjacency_list[],
                               const std::vector<uint32_t> &vertices,
                               std::size_t max_clique_size,
                               std::vector<std::vector<uint32_t>> &out_cliques);

void bron_kerbosch_max_neighborhood_pivot(const std::vector<uint32_t> adjacency_list[],
                                          const std::vector<uint32_t> &vertices,
                                          std::size_t max_clique_size,
                                          std::vector<std::vector<uint32_t>> &out_cliques);

void bron_kerbosch_min_difference_pivot(const std::vector<uint32_t> adjacency_list[],
                                        const std::vector<uint32_t> &vertices,
                                        std::size_t max_clique_size,
                                        std::vector<std::vector<uint32_t>> &out_cliques);

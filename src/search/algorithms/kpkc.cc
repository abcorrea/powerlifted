#include "kpkc.h"

namespace algorithms {

bool find_all_k_cliques_in_k_partite_graph_helper(const std::vector<boost::dynamic_bitset<>> &adjacency_matrix,
                                                  const std::vector<std::vector<size_t>> &partitions,
                                                  std::vector<boost::dynamic_bitset<>> &compatible_vertices,
                                                  boost::dynamic_bitset<> &partition_bits,
                                                  boost::dynamic_bitset<> &not_partition_bits,
                                                  std::vector<uint32_t> &partial_solution,
                                                  std::vector<std::vector<uint32_t>> &out_cliques) {
    size_t k = partitions.size();
    size_t best_set_bits = std::numeric_limits<size_t>::max();
    size_t best_partition = std::numeric_limits<size_t>::max();

    // Find the best partition to work with
    for (size_t partition = 0; partition < k; ++partition) {
        const auto num_set_bits = compatible_vertices[partition].count();

        if (not_partition_bits[partition] && (num_set_bits < best_set_bits)) {
            best_set_bits = num_set_bits;
            best_partition = partition;
        }
    }

    size_t adjacent_index = compatible_vertices[best_partition].find_first();

    // Iterate through compatible vertices in the best partition
    while (adjacent_index < compatible_vertices[best_partition].size()) {
        size_t vertex = partitions[best_partition][adjacent_index];
        compatible_vertices[best_partition][adjacent_index] = 0;
        partial_solution.push_back(static_cast<uint32_t>(vertex));

        if (partial_solution.size() == k) {
            out_cliques.push_back(partial_solution);
        }
        else {
            // Update compatible vertices for the next recursion
            std::vector<boost::dynamic_bitset<>> compatible_vertices_next = compatible_vertices;
            size_t offset = 0;
            for (size_t partition = 0; partition < k; ++partition) {
                const auto partition_size = compatible_vertices_next[partition].size();
                if (not_partition_bits[partition]) {
                    for (size_t index = 0; index < partition_size; ++index) {
                        compatible_vertices_next[partition][index] &=
                            adjacency_matrix[vertex][index + offset];
                    }
                }
                offset += partition_size;
            }

            partition_bits[best_partition] = 1;
            not_partition_bits[best_partition] = 0;

            size_t possible_additions = 0;
            for (size_t partition = 0; partition < k; ++partition) {
                if (not_partition_bits[partition] && compatible_vertices[partition].any()) {
                    ++possible_additions;
                }
            }

            if ((partial_solution.size() + possible_additions) == k) {
                if (!find_all_k_cliques_in_k_partite_graph_helper(adjacency_matrix,
                                                                  partitions,
                                                                  compatible_vertices_next,
                                                                  partition_bits,
                                                                  not_partition_bits,
                                                                  partial_solution,
                                                                  out_cliques)) {
                    return false;
                }
            }

            partition_bits[best_partition] = 0;
            not_partition_bits[best_partition] = 1;
        }

        partial_solution.pop_back();
        adjacent_index = compatible_vertices[best_partition].find_next(adjacent_index);
    }

    return true;
}

bool find_all_k_cliques_in_k_partite_graph(const std::vector<boost::dynamic_bitset<>> &adjacency_matrix,
                                           const std::vector<std::vector<size_t>> &partitions,
                                           std::vector<std::vector<uint32_t>> &out_cliques) {
    std::vector<boost::dynamic_bitset<>> compatible_vertices;

    for (std::size_t index = 0; index < partitions.size(); ++index) {
        boost::dynamic_bitset<> bitset(partitions[index].size());
        bitset.set();
        compatible_vertices.push_back(std::move(bitset));
    }

    const size_t k = partitions.size();
    boost::dynamic_bitset<> partition_bits(k);
    boost::dynamic_bitset<> not_partition_bits(k);
    partition_bits.reset();
    not_partition_bits.set();
    std::vector<uint32_t> partial_solution;

    const auto finished = find_all_k_cliques_in_k_partite_graph_helper(adjacency_matrix,
                                                                       partitions,
                                                                       compatible_vertices,
                                                                       partition_bits,
                                                                       not_partition_bits,
                                                                       partial_solution,
                                                                       out_cliques);

    return finished;
}

}  // namespace algorithms


#pragma once

#include "../utils/segmented_vector.h"

#include <ctime>
#include <utility>
#include <unordered_map>
#include <vector>

class LiftedOperatorId;
class PackedStateHash;
class SuccessorGenerator;
class SparsePackedState;
class SparseStatePacker;
class Task;


void print_no_solution_found(const clock_t& timer_start);

void print_goal_found(
    const SuccessorGenerator& generator,
    const clock_t& timer_start);


void extract_plan(
    segmented_vector::SegmentedVector<std::pair<int, LiftedOperatorId>> &cheapest_parent,
    SparsePackedState state,
    const std::unordered_map<SparsePackedState, int, PackedStateHash> &visited,
    segmented_vector::SegmentedVector<SparsePackedState> &index_to_state,
    const SparseStatePacker &packer,
    const Task &task);

void print_plan(const std::vector<LiftedOperatorId>& plan, const Task &task);


#pragma once

#include "../utils/segmented_vector.h"

#include <ctime>
#include <utility>

class LiftedOperatorId;
class PackedStateHash;
class SuccessorGenerator;
class SparsePackedState;
class SparseStatePacker;
class Task;


void print_no_solution_found(clock_t timer_start);

void print_goal_found(
    const SuccessorGenerator& generator,
    const clock_t& timer_start,
    int generations_until_last_jump);


void extract_plan(
    segmented_vector::SegmentedVector<std::pair<int, LiftedOperatorId>> &cheapest_parent,
    SparsePackedState state,
    const std::unordered_map<SparsePackedState, int, PackedStateHash> &visited,
    segmented_vector::SegmentedVector<SparsePackedState> &index_to_state,
    const SparseStatePacker &packer,
    const Task &task);
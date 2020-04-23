cmake_minimum_required(VERSION 3.9)
project(search)

set(CMAKE_CXX_STANDARD 17)

if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES Debug Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wnon-virtual-dtor")

# Release
set(CMAKE_CXX_FLAGS_RELEASE
        "-O3 -g -DNDEBUG -fomit-frame-pointer -Werror")

# Debug and Profile
set(CMAKE_CXX_FLAGS_DEBUG
        "${CMAKE_CXX_FLAGS_DEBUG} -O0 -D_GLIBCXX_DEBUG -pg")

add_executable(search
        main.cc
        task.cc task.h
        predicate.cc predicate.h
        object.h
        states/state.cc states/state.h
        parser.cc parser.h
        goal_condition.h
        action_schema.cc action_schema.h
        structures.h
        search_engines/search.cc search_engines/search.h
        action.h
        successor_generators/successor_generator.cc successor_generators/successor_generator.h
        database/table.h
        database/join.cc database/join.h
        heuristics/goalcount.cc heuristics/goalcount.h
        search_engines/breadth_first_search.cc search_engines/breadth_first_search.h
        search_engines/search_factory
        heuristics/heuristic.h
        heuristics/heuristic_factory.h
        heuristics/blind_heuristic.h
        search_engines/greedy_best_first_search.cc search_engines/greedy_best_first_search.h
        successor_generators/successor_generator_factory.h
        successor_generators/naive_successor.cc successor_generators/naive_successor.h
        successor_generators/ordered_join_successor.cc successor_generators/ordered_join_successor.h
        successor_generators/inverse_ordered_join_successor.cc successor_generators/inverse_ordered_join_successor.h
        successor_generators/generic_join_successor.cc successor_generators/generic_join_successor.h
        successor_generators/full_reducer_successor_generator.cc successor_generators/full_reducer_successor_generator.h
        database/semi_join.h database/semi_join.cc
        database/hash_join.cc database/hash_join.h
        hash_structures.h
        database/hash_semi_join.cc database/hash_semi_join.h
        utils.cc utils.h
        successor_generators/random_successor.h successor_generators/random_successor.cc
        successor_generators/yannakakis.cc successor_generators/yannakakis.h
        database/project.cc database/project.h
        utils/segmented_vector.h
        states/extensional_states
        states/sparse_states
        utils/hash.h
        utils/cartesian_iterator.h
)

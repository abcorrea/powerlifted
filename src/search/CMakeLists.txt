cmake_minimum_required(VERSION 3.9)
project(search)

set(CMAKE_CXX_STANDARD 17)


if (CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES Debug Release)
endif ()

find_package(Boost COMPONENTS program_options REQUIRED)
if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    add_definitions( "-DHAS_BOOST" )
endif()


set(CMAKE_CXX_FLAGS "-Wall -Wnon-virtual-dtor")

# Release
set(CMAKE_CXX_FLAGS_RELEASE
        "-O3 -g -DNDEBUG -fomit-frame-pointer -Werror")

# Debug and Profile
set(CMAKE_CXX_FLAGS_DEBUG
        "${CMAKE_CXX_FLAGS_DEBUG} -O0 -pg \
        -fsanitize=undefined -fno-omit-frame-pointer")

# Process Abseil's CMake build system
add_subdirectory(abseil-cpp)


add_executable(search
        main.cc
        task.cc task.h
        predicate.cc predicate.h
        object.h
        states/state.cc states/state.h
        parser.cc parser.h
        goal_condition.h
        atom.h
        action_schema.cc action_schema.h
        structures.h
        search_engines/search_factory
        search_engines/search
        search_engines/breadth_first_search
        search_engines/greedy_best_first_search
        search_engines/nodes
        search_engines/utils
        search_engines/search_space
        action
        successor_generators/successor_generator.h
        database/table
        database/join
        database/utils
        heuristics/goalcount
        heuristics/heuristic.h
        heuristics/heuristic_factory
        heuristics/blind_heuristic.h
        successor_generators/successor_generator_factory
        successor_generators/naive_successor.h
        successor_generators/ordered_join_successor.cc successor_generators/ordered_join_successor.h
        successor_generators/generic_join_successor.cc successor_generators/generic_join_successor.h
        successor_generators/full_reducer_successor_generator.cc successor_generators/full_reducer_successor_generator.h
        database/semi_join.h database/semi_join.cc
        database/hash_join.cc database/hash_join.h
        hash_structures
        database/hash_semi_join.cc database/hash_semi_join.h
        utils.cc utils.h
        successor_generators/random_successor.h successor_generators/random_successor.cc
        successor_generators/yannakakis.cc successor_generators/yannakakis.h
        database/project.cc database/project.h
        utils/segmented_vector.h
        states/extensional_states
        states/sparse_states
        utils/hash.h
        algorithms/cartesian_iterator.h
        utils/collections.h
        utils/language.h
        utils/system
        utils/system_unix
        utils/system_windows
        utils/logging
        utils/timer
        algorithms/int_hash_set.h
        algorithms/dynamic_bitset.h
        search_statistics
        options.h open_lists/greedy_open_list.h
        search_engines/lazy_search.cc search_engines/lazy_search.h
        datalog/datalog.cc datalog/datalog.h
        datalog/arguments.h
        datalog/datalog_atom.cc datalog/datalog_atom.h
        datalog/datalog_fact.cc datalog/datalog_fact.h
        datalog/term.h
        datalog/object.cc datalog/object.h
        datalog/rules/rule_base.cc datalog/rules/rule_base.h
        delete_relaxation_heuristics/delete_relaxation_heuristic.h
        datalog/rules/generic_rule.h
        datalog/rules/join.h
        datalog/rules/product.h
        datalog/rules/project.h
        heuristics/ff_heuristic.h
        datalog/annotations/annotation.h
        datalog/ground_rule.h
        heuristics/ff_heuristic.cc
        heuristics/rff_heuristic.cc
        heuristics/rff_heuristic.h
        heuristics/datalog_transformation_options.h
        datalog/achievers.h
        datalog/transformations/action_predicate_removal.h
        datalog/transformations/normal_form.h
        datalog/transformations/greedy_join.h
        datalog/transformations/goal_rule.h
        datalog/transformations/generate_edb.h datalog/rules/variable_source.h
        datalog/grounder/grounder.h
        datalog/grounder/weighted_grounder.cc datalog/grounder/weighted_grounder.h
        datalog/rule_matcher.cc datalog/rule_matcher.h heuristics/add_heuristic.cc
        heuristics/add_heuristic.h heuristics/utils.h heuristics/utils.cc
        datalog/transformations/remove_equivalent_rules.h datalog/transformations/connected_components.h
        datalog/transformations/variable_projection.h datalog/transformations/variable_renaming.h
        )

target_link_libraries(search LINK_PUBLIC ${Boost_LIBRARIES} absl::flat_hash_map)

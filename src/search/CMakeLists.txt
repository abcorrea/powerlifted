cmake_minimum_required(VERSION 3.9)
project(search)

set(CMAKE_CXX_STANDARD 17)


if (CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES Debug Release)
endif ()

find_package(Boost COMPONENTS program_options REQUIRED CONFIG)
if (Boost_FOUND)
    add_definitions( "-DHAS_BOOST" )
endif()


set(CMAKE_CXX_FLAGS "-Wall -Wnon-virtual-dtor")

if (APPLE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Applying macOS/Clang-specific compiler options")
    add_compile_options(-Wno-deprecated-declarations)
    add_compile_options(-Wno-error=deprecated-builtins)
    add_compile_options(-Wno-deprecated-builtins)
    add_compile_options(-std=gnu++17 -Wno-error=deprecated-builtins -Wno-vla)
endif()


# Release
set(CMAKE_CXX_FLAGS_RELEASE
        "-O3 -g -DNDEBUG -fomit-frame-pointer -Werror")

# Debug and Profile
set(CMAKE_CXX_FLAGS_DEBUG
        "${CMAKE_CXX_FLAGS_DEBUG} -O0 -pg \
        -fsanitize=undefined -fno-omit-frame-pointer")

add_executable(search
        main.cc
        task.cc task.h
        predicate.cc predicate.h
        object.h
        plan_manager.cc plan_manager.h
        states/state.cc states/state.h
        parser.cc parser.h
        goal_condition.h
        atom.h
        action_schema.cc action_schema.h
        structures.h
        search_engines/search_factory.cc search_engines/search_factory.h
        search_engines/search.cc search_engines/search.h
        search_engines/breadth_first_search.cc search_engines/breadth_first_search.h
        search_engines/greedy_best_first_search.cc search_engines/greedy_best_first_search.h
        search_engines/nodes.cc search_engines/nodes.h
        search_engines/utils.cc search_engines/utils.h
        search_engines/search_space.cc search_engines/search_space.h
        action.cc action.h
        successor_generators/successor_generator.h
        database/table.cc database/table.h
        database/join.cc database/join.h
        database/utils.cc database/utils.h
        heuristics/goalcount.cc  heuristics/goalcount.h
        heuristics/heuristic.h
        heuristics/heuristic_factory.cc heuristics/heuristic_factory.h
        heuristics/blind_heuristic.h
        successor_generators/successor_generator_factory.cc successor_generators/successor_generator_factory.h
        successor_generators/naive_successor.h
        successor_generators/ordered_join_successor.cc successor_generators/ordered_join_successor.h
        successor_generators/generic_join_successor.cc successor_generators/generic_join_successor.h
        successor_generators/full_reducer_successor_generator.cc successor_generators/full_reducer_successor_generator.h
        database/semi_join.h database/semi_join.cc
        database/hash_join.cc database/hash_join.h
        hash_structures.cc hash_structures.h
        database/hash_semi_join.cc database/hash_semi_join.h
        utils.cc utils.h
        successor_generators/random_successor.h successor_generators/random_successor.cc
        successor_generators/yannakakis.cc successor_generators/yannakakis.h
        successor_generators/clique_successor_generator.cc successor_generators/clique_successor_generator_apply.cc successor_generators/clique_successor_generator.h
        successor_generators/clique_bron_kerbosch.cc successor_generators/clique_bron_kerbosch.h
        successor_generators/clique_help_functions.cc successor_generators/clique_help_functions.h
        database/project.cc database/project.h
        utils/segmented_vector.h
        states/sparse_states.cc states/sparse_states.h
        utils/hash.h
        algorithms/cartesian_iterator.h
        utils/collections.h
        utils/language.h
        utils/system.cc utils/system.h
        utils/system_unix.cc utils/system_unix.h
        utils/system_windows.cc utils/system_windows.h
        utils/logging.cc utils/logging.h
        utils/timer.cc utils/timer.h
        algorithms/int_hash_set.h
        algorithms/dynamic_bitset.h
        algorithms/kpkc.cc algorithms/kpkc.h
        search_statistics.cc search_statistics.h
        options.h open_lists/greedy_open_list.h
        search_engines/best_first_width_search.cc search_engines/best_first_width_search.h
        novelty/standard_novelty.cc novelty/standard_novelty.h
        open_lists/tiebreaking_open_list.h
        novelty/achieved_ground_atoms.h
        novelty/atom_counter.h
        search_engines/dual_queue_bfws.cc search_engines/dual_queue_bfws.h
        novelty/node_novelty.h
        search_engines/alternated_bfws.cc search_engines/alternated_bfws.h
        search_engines/lazy_search.cc search_engines/lazy_search.h
        search_engines/lazy_search.cc search_engines/lazy_search.h
        search_engines/astar_search.cc search_engines/astar_search.h
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
        datalog/transformations/variable_projection.h datalog/transformations/variable_renaming.h heuristics/hmax_heuristic.cc heuristics/hmax_heuristic.h
        parallel_hashmap/phmap.h)

target_link_libraries(search LINK_PUBLIC ${Boost_LIBRARIES})

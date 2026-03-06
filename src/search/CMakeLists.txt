cmake_minimum_required(VERSION 3.14)
project(search LANGUAGES CXX)

# --- Source files ---

set(CORE_SOURCES
        main.cc
        action.cc
        action_schema.cc
        hash_structures.cc
        parser.cc
        plan_manager.cc
        predicate.cc
        search_statistics.cc
        task.cc
        utils.cc)

set(DATABASE_SOURCES
        database/hash_join.cc
        database/hash_semi_join.cc
        database/project.cc
        database/semi_join.cc
        database/table.cc
        database/utils.cc)

set(DATALOG_SOURCES
        datalog/datalog.cc
        datalog/datalog_atom.cc
        datalog/datalog_fact.cc
        datalog/object.cc
        datalog/grounder/weighted_grounder.cc
        datalog/rule_matcher.cc
        datalog/rules/rule_base.cc)

set(HEURISTIC_SOURCES
        heuristics/add_heuristic.cc
        heuristics/ff_heuristic.cc
        heuristics/goalcount.cc
        heuristics/heuristic_factory.cc
        heuristics/hmax_heuristic.cc
        heuristics/rff_heuristic.cc
        heuristics/utils.cc)

set(NOVELTY_SOURCES
        novelty/standard_novelty.cc)

set(SEARCH_ENGINE_SOURCES
        search_engines/alternated_bfws.cc
        search_engines/astar_search.cc
        search_engines/best_first_width_search.cc
        search_engines/breadth_first_search.cc
        search_engines/dual_queue_bfws.cc
        search_engines/greedy_best_first_search.cc
        search_engines/lazy_search.cc
        search_engines/nodes.cc
        search_engines/search.cc
        search_engines/search_factory.cc
        search_engines/search_space.cc
        search_engines/utils.cc)

set(STATE_SOURCES
        states/sparse_states.cc
        states/state.cc)

set(SUCCESSOR_GENERATOR_SOURCES
        successor_generators/clique_bron_kerbosch.cc
        successor_generators/clique_help_functions.cc
        successor_generators/clique_successor_generator.cc
        successor_generators/clique_successor_generator_apply.cc
        successor_generators/full_reducer_successor_generator.cc
        successor_generators/generic_join_successor.cc
        successor_generators/ordered_join_successor.cc
        successor_generators/random_successor.cc
        successor_generators/successor_generator_factory.cc
        successor_generators/yannakakis.cc)

set(UTIL_SOURCES
        utils/logging.cc
        utils/system.cc
        utils/system_unix.cc
        utils/system_windows.cc
        utils/timer.cc
        algorithms/kpkc.cc)

add_executable(search
        ${CORE_SOURCES}
        ${DATABASE_SOURCES}
        ${DATALOG_SOURCES}
        ${HEURISTIC_SOURCES}
        ${NOVELTY_SOURCES}
        ${SEARCH_ENGINE_SOURCES}
        ${STATE_SOURCES}
        ${SUCCESSOR_GENERATOR_SOURCES}
        ${UTIL_SOURCES})

# --- Compiler settings ---

target_compile_features(search PUBLIC cxx_std_17)
set_target_properties(search PROPERTIES
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF)

target_compile_options(search PRIVATE
        -Wall
        -Wnon-virtual-dtor)

if (APPLE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Applying macOS/Clang-specific compiler options")
    target_compile_options(search PRIVATE
            -Wno-deprecated-declarations
            -Wno-deprecated-builtins
            -Wno-vla)
endif()

# --- Build-type specific flags ---

target_compile_options(search PRIVATE
        $<$<CONFIG:Release>:-O3 -g -DNDEBUG -fomit-frame-pointer -Werror>
        $<$<CONFIG:Debug>:-O0 -pg -fsanitize=undefined -fno-omit-frame-pointer>)

target_link_options(search PRIVATE
        $<$<CONFIG:Debug>:-pg -fsanitize=undefined>)

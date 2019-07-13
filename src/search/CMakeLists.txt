cmake_minimum_required(VERSION 3.10)
project(search)

set(CMAKE_CXX_STANDARD 17)

#set(CMAKE_CXX_FLAGS "-O3")

add_executable(search
        main.cc
        task.cc task.h
        predicate.cc predicate.h
        object.cc object.h
        state.cc state.h
        parser.cc parser.h
        goal_condition.cc goal_condition.h
        action_schema.cc action_schema.h
        structures.h
        search_engines/search.cc search_engines/search.h
        action.h
        successor_generators/successor_generator.cc successor_generators/successor_generator.h
        database/table.cc database/table.h
        database/join.h
        heuristics/goalcount.cc heuristics/goalcount.h
        search_engines/breadth_first_search.cc search_engines/breadth_first_search.h
        search_engines/search_factory.h
        search_engines/astar_search.cc search_engines/astar_search.h
        heuristics/heuristic.cc heuristics/heuristic.h
        heuristics/heuristic_factory.h
        heuristics/blind_heuristic.h search_engines/greedy_best_first_search.cc search_engines/greedy_best_first_search.h successor_generators/successor_generator_factory.h successor_generators/naive_successor.cc successor_generators/naive_successor.h successor_generators/ordered_join_successor.cc successor_generators/ordered_join_successor.h database/join.cc)
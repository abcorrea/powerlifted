cmake_minimum_required(VERSION 3.14)
project(search)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "-O3")

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
        search.cc search.h
        action.h
        successor_generators/successor_generator.cc successor_generators/successor_generator.h
        database/table.cc database/table.h
        database/join.h heuristics/goalcount.cc heuristics/goalcount.h)
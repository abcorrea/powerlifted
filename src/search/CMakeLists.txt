cmake_minimum_required(VERSION 3.14)
project(search)

set(CMAKE_CXX_STANDARD 17)

add_executable(search
        main.cc
        task.cc task.h
        predicate.cc predicate.h
        object.cc object.h
        state.cc state.h)
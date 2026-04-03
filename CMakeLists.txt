cmake_minimum_required(VERSION 3.20)
project(PL-JIT-Compiler)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(interpreter src/main.cpp
        src/helpers/io_helpers.cpp
        src/parse_into_ast.cpp
        src/helpers/debug_helpers.cpp
        src/tree_interpreter.cpp
        src/tree_interpreter.h)
target_include_directories(interpreter PRIVATE third_party)
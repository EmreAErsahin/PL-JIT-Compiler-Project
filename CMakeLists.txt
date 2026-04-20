cmake_minimum_required(VERSION 3.20)
project(PL-JIT-Compiler LANGUAGES CXX)

add_executable(interpreter
        src/main.cpp
        src/helpers/io_helpers.cpp
        src/parse_into_ast.cpp
        src/helpers/debug_helpers.cpp
        src/tree_interpreter.cpp
)

target_include_directories(interpreter PRIVATE third_party)

target_compile_features(interpreter PRIVATE cxx_std_23)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(interpreter PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif ()

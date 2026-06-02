cmake_minimum_required(VERSION 3.20)

project(PL-JIT-Compiler LANGUAGES CXX)

# All the flags we want to apply to each compilation
function(enable_project_warnings target_name)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target_name} PRIVATE
                -Wall
                -Wextra
                -Wpedantic
        )
    endif ()
endfunction()

# Adds all passing / failing tests to the list of executables upon creation/edit
function(add_embedding_tests test_kind)
    file(GLOB test_sources CONFIGURE_DEPENDS tests/embedding_tests/${test_kind}/*.cpp)

    foreach (test_source ${test_sources})
        get_filename_component(test_name ${test_source} NAME_WE)
        set(target_name embedding_${test_kind}_${test_name})
        add_executable(${target_name} ${test_source})
        target_link_libraries(${target_name} PRIVATE ee_runtime)
        set_target_properties(${target_name} PROPERTIES
                OUTPUT_NAME ${test_name}
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/embedding_tests/${test_kind}"
        )
        enable_project_warnings(${target_name})
        list(APPEND EMBEDDING_TEST_TARGETS ${target_name})
    endforeach ()

    set(EMBEDDING_TEST_TARGETS ${EMBEDDING_TEST_TARGETS} PARENT_SCOPE)
endfunction()

# Builds reusable ee library
add_library(ee_runtime
        src/parser/parser.cpp
        src/ast/ast_printer.cpp
        src/tree_interpreter/runtime_state.cpp
        src/tree_interpreter/runtime_value.cpp
        src/tree_interpreter/tree_interpreter.cpp
        src/vm/vm.cpp
)
target_include_directories(ee_runtime PUBLIC src third_party)
target_compile_features(ee_runtime PUBLIC cxx_std_23)
enable_project_warnings(ee_runtime)

# Builds the CLI interpreter executable.
add_executable(interpreter
        src/main.cpp
)
set_target_properties(interpreter PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/interpreter"
)
target_link_libraries(interpreter PRIVATE ee_runtime)
enable_project_warnings(interpreter)

# Builds each separate executable for the embedding tests
set(EMBEDDING_TEST_TARGETS)
add_embedding_tests(pass)
add_embedding_tests(fail)
add_custom_target(embedding_tests DEPENDS ${EMBEDDING_TEST_TARGETS})

# PL-JIT-Compiler-Project

Building a programming language from scratch in C++. Starting with a tree-walk interpreter, with plans to add a garbage
collector and JIT compiler.

## Build Details

- The project is built with CMake and a top-level Makefile wrapper
- To run the project
    - Build the project: `make build`
    - Run the interpreter directly: `./build/interpreter SOURCE`
    - Run with AST debug output: `./build/interpreter --debug SOURCE`
    - Run a single passing test: `./scripts/run_single_test.sh tests/pass/<test_name>.ee`
    - Run all passing tests: `./scripts/run_passing_tests.sh`
    - Run all failing tests: `./scripts/run_failing_tests.sh`
    - Run the full test suite: `./scripts/run_all_tests.sh`
- Other options
    - `make clean` -> cleans the `build/` directory

## PL Grammar (cpp-peglib preferred format)

Program      <- Function

Function     <- Type Identifier '(' ')' '{' Body '}'

Body         <- Instruction*

Instruction  <- Print ';'

Print        <- 'print' '(' Integer? ')'

Type         <- 'void'

Identifier   <- < [a-zA-Z_][a-zA-Z0-9_]* >

Integer      <- < '-'? [0-9]+ >

%whitespace  <- [ \t\r\n]*

## Language Semantics

- `print(<integer>)` behavior: Prints the value with no automatic newline
- `print()` behavior: Prints nothing

## Invariants

- For the program to run there must be a function called main. This is the entry point
- The interpreter takes exactly one source file path, with optional `--debug` before it

## Future Features

- Garbage collector
- JIT compiler

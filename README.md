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

## Current Grammar (cpp-peglib preferred format)

Program      <- Function

Function     <- 'fn' Identifier '(' ')' '{' Statement* '}'

Statement    <- Print ';'

Print        <- 'print' '(' Expression? ')'

Expression   <- Primary

Primary      <- Integer / Bool / Nothing

Bool         <- 'true' / 'false'

Nothing      <- 'nothing'

Identifier   <- < [a-zA-Z_][a-zA-Z0-9_]* >

Integer      <- < '-'? [0-9]+ >

%whitespace  <- [ \t\r\n]*

## Stage 1 Grammar (Not yet implemented pieces preceded with ---)

Program         <- Function
Function        <- 'fn' Identifier '(' ')' Block
--- Block           <- '{' Statement* '}'
--- Statement       <- PrintStmt / LetStmt / IfStmt
--- PrintStmt       <- 'print' '(' Expression? ')' ';'
--- LetStmt         <- 'let' Identifier '=' Expression ';'
--- IfStmt          <- 'if' '(' Expression ')' Block
--- Expression      <- Additive
--- Additive        <- Multiplicative (('+' / '-') Multiplicative)*
--- Multiplicative  <- Unary (('*' / '/') Unary)*
--- Unary           <- Primary
--- Primary         <- Integer / Bool / Nothing / Identifier / '(' Expression ')'
--- Bool            <- 'true' / 'false'
--- Nothing         <- 'nothing'
Identifier      <- < [a-zA-Z_][a-zA-Z0-9_]* >
Integer         <- < '-'? [0-9]+ >

## Language Semantics

- `print(<integer>)` behavior: Prints the value with no automatic newline
- `print()` behavior: Prints nothing
- Functions are declared with `fn`, for example `fn main() { ... }`

## Invariants

- For the program to run there must be a function called main. This is the entry point
- The interpreter takes exactly one source file path, with optional `--debug` before it

## Future Features

- Garbage collector
- JIT compiler

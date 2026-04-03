# PL-JIT-Compiler-Project

Building a programming language from scratch in C++. Starting with a tree-walk interpreter, with plans to add a garbage
collector and JIT compiler.

## Build Details

- The project is built with CMake & a Makefile wrapper to easily use Cmake
- To run the project
    - Make run from the root directory of the project
- Other options
    - Make clean -> (cleans build folder)
    - Make build -> (builds the project)

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

## Future Features

- Garbage collector
- JIT compiler

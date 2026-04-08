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

Statement    <- DebugPrint ';'

DebugPrint   <- 'debugPrint' '(' Expression? ')'

Expression   <- InfixExpression(Atom, ArithmeticOperator)

Atom         <- Integer / Bool / Nothing / '(' Expression ')'

ArithmeticOperator <- < [-+/*] >

Bool         <- 'true' / 'false'

Nothing      <- 'nothing'

Identifier   <- < [a-zA-Z_][a-zA-Z0-9_]* >

Integer      <- < '-'? [0-9]+ >

InfixExpression(A, O) <- A (O A)* {
  precedence
    L + -
    L * /
}

%whitespace  <- [ \t\r\n]*

## Stage 1 Grammar (Not yet implemented pieces preceded with ---)

Program         <- Function
Function        <- 'fn' Identifier '(' ')' Block
--- Block           <- '{' Statement* '}'
--- Statement       <- DebugPrintStmt / LetStmt / IfStmt
--- DebugPrintStmt  <- 'debugPrint' '(' Expression? ')' ';'
--- LetStmt         <- 'let' Identifier '=' Expression ';'
--- IfStmt          <- 'if' '(' Expression ')' Block
Expression      <- InfixExpression(Atom, ArithmeticOperator)
Atom            <- Integer / Bool / Nothing / '(' Expression ')'
ArithmeticOperator <- < [-+/*] >
InfixExpression(A, O) <- A (O A)* {
  precedence
    L + -
    L * /
}
--- IdentifierExpr  <- Identifier
--- Bool            <- 'true' / 'false'
--- Nothing         <- 'nothing'
Identifier      <- < [a-zA-Z_][a-zA-Z0-9_]* >
Integer         <- < '-'? [0-9]+ >

## Language Semantics

- `debugPrint(<integer>)` behavior: Prints the value with no automatic newline
- `debugPrint(<bool>)` prints `true` or `false`
- `debugPrint(nothing)` prints `nothing`
- `debugPrint()` behavior: Prints nothing
- Arithmetic currently supports `+`, `-`, `*`, and `/` on integers only
- Parentheses are supported in arithmetic expressions
- Functions are declared with `fn`, for example `fn main() { ... }`

## Invariants

- For the program to run there must be a function called main. This is the entry point
- The interpreter takes exactly one source file path, with optional `--debug` before it

## Future Features

- Garbage collector
- JIT compiler

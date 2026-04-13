# PL-JIT-Compiler-Project

Building a programming language from scratch in C++. Starting with a tree-walk interpreter, with plans to add a garbage
collector and JIT compiler.

## Build Details

- The project is built with CMake and a top-level Makefile wrapper
- To run the project
    - Build the project: `make build`
    - Run the interpreter directly: `./build/interpreter SOURCE`
    - Run a single passing test: `./scripts/run_single_test.sh tests/pass/<test_name>.ee`
    - Run all passing tests: `./scripts/run_passing_tests.sh`
    - Run all failing tests: `./scripts/run_failing_tests.sh`
    - Run the full test suite: `./scripts/run_all_tests.sh`
    - Run a test with AST debug output: `./scripts/debug_test.sh tests/pass/<test_name>.ee`
- Other options
    - `make clean` -> cleans the `build/` directory

## Current Grammar (cpp-peglib preferred format)

Program      <- Function

Function     <- 'fn' Identifier '(' ')' '{' Statement* '}'

Statement    <- DebugPrintStatement / LetStatement / AssignmentStatement

DebugPrintStatement <- 'debugPrint' '(' Expression? ')' ';'

LetStatement <- 'let' Identifier '=' Expression ';'

AssignmentStatement <- Identifier '=' Expression ';'

Expression   <- InfixExpression(Atom, ArithmeticOperator)

Atom         <- Integer / Bool / Nothing / IdentifierExpr / '(' Expression ')'

ArithmeticOperator <- < [-+/*] >

Bool         <- 'true' / 'false'

Nothing      <- 'nothing'

Identifier   <- < [a-zA-Z_][a-zA-Z0-9_]* >

Integer      <- < '-'? [0-9]+ >

IdentifierExpr <- Identifier

InfixExpression(A, O) <- A (O A)* {
precedence
L + -
L * /
}

%whitespace  <- [ \t\r\n]*

## Language Semantics

- `debugPrint(<integer>)` behavior: Prints the value with no automatic newline
- `debugPrint(<bool>)` prints `true` or `false`
- `debugPrint(nothing)` prints `nothing`
- `debugPrint()` behavior: Prints nothing
- Arithmetic currently supports `+`, `-`, `*`, and `/` on integers only
- Parentheses are supported in arithmetic expressions
- Integer literals may be negative, for example `-5`
- Variables are introduced with `let`, for example `let x = 1;`
- Existing variables can be reassigned with `x = expression;`
- Variables can be used anywhere expressions are allowed
- Functions are declared with `fn`, for example `fn main() { ... }`

## Invariants

- For the program to run there must be a function called main. This is the entry point
- Variables must be initialized when declared; bare declarations are not supported
- Reassignment requires the variable to already exist
- The interpreter takes exactly one source file path, with optional `--debug` flag for AST debug output

## Future Features

- Garbage collector
- JIT compiler

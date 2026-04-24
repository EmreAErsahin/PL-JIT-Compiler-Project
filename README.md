# PL-JIT-Compiler-Project

Small C++23 language implementation project built around a handwritten AST, a `cpp-peglib` parser, and a tree-walk interpreter. The repository is still in an interpreter-first phase, but it already supports more than the original arithmetic-only subset.

## What The Code Implements Today

The current implementation is a direct parser -> AST -> tree interpreter pipeline with these language features:

- Multiple top-level function definitions
- Required `fn main()` in CLI execution
- `print(...)` with an optional expression and no automatic newline
- `let` declarations with required initializers
- Assignment to an existing variable
- `return` with and without a value
- Integer, boolean, and `nothing` literals
- Identifier expressions
- Zero-argument function calls in expression and statement position
- Arithmetic `+`, `-`, `*`, `/` on integers only
- Relational `<`, `<=`, `>`, `>=` on integers only
- Equality `==`, `!=` across all current runtime value types
- Logical `&&`, `||` with short-circuit behavior
- Parenthesized expressions
- Nested `{ ... }` blocks
- `if` / `else if` / `else`
- `while`
- C-style `for let i = init; condition; i = update { ... }`
- `break`
- `continue`
- `//` line comments
- Lexical block scoping with shadowing

Current implementation limits:

- Functions do not yet support parameters
- Function calls are currently zero-argument only
- Functions implicitly return `nothing` when no `return` is executed
- There are no arrays, tables, strings, floats, closures, unary operators, modulo, or expression statements yet
- Runtime values are currently only `int64_t`, `bool`, and `nothing`
- Truthiness currently treats `false`, `nothing`, and integer `0` as falsy; nonzero integers and `true` are truthy
- Logical operators return booleans, not original operands
- Equality on mismatched runtime types evaluates to `false`
- `break` and `continue` outside loops are detected at runtime, not during parsing
- Parse and runtime failures are surfaced as `std::runtime_error` messages printed to `stderr`

## Architecture

- `src/parser/language_grammar.h`: PEG grammar string
- `src/parser/parser.cpp`: parser construction and semantic actions that build the AST
- `src/ast/ast.h`: AST node definitions
- `src/ast/ast_printer.cpp`: source-like AST pretty printer used by `--debug`
- `src/tree_interpreter/tree_interpreter.cpp`: runtime value model, scope handling, and execution
- `src/main.cpp`: CLI entry point, file loading, `--debug`, and top-level error reporting

Implementation notes from the current code:

- The parser uses `cpp-peglib` semantic values stored as `std::any`, which is why some AST children are held via `std::shared_ptr`
- Functions are collected into a top-level function table before execution, so later-defined functions may be called earlier
- Function calls execute in fresh call frames with their own scope stacks
- Function execution converts statement-level control flow into final function results, with leaked `break` / `continue` rejected at the function boundary
- `for` loop initializer scope is intentionally kept alive for the full loop
- In `for`, `continue` still runs the update step before the next condition check
- `--debug` prints the parsed AST back into a normalized source-like form before execution

## Build

```sh
make build
```

Direct CMake is also supported:

```sh
cmake -S . -B build
cmake --build build
```

Compiler/build details from the current build config:

- CMake minimum version: `3.20`
- Executable target: `interpreter`
- Language standard: `C++23`
- Clang/GCC warnings: `-Wall -Wextra -Wpedantic`

## Run

```sh
./build/interpreter SOURCE
./build/interpreter --debug SOURCE
```

CLI notes:

- `SOURCE` must be a file path
- `--debug` prints the AST and then executes the program
- Invalid CLI usage prints `Usage: <program> [--debug] SOURCE`

## Tests

The test suite is golden-file based.

- Passing tests live in `tests/pass` as `.ee` + matching `.out`
- Failing tests live in `tests/fail` as `.ee` + matching `.err`
- Output and error text are exact-match checked with `diff`

Useful commands:

```sh
./scripts/run_single_test.sh tests/pass/<name>.ee
./scripts/run_single_test.sh tests/fail/<name>.ee
./scripts/run_passing_tests.sh
./scripts/run_failing_tests.sh
./scripts/run_all_tests.sh
./scripts/debug_test.sh tests/pass/<name>.ee
```

## Current Grammar

```peg
Program                    <- Function+
Keyword                    <- KeywordFn / KeywordLet / KeywordPrint / KeywordIf / KeywordElse / KeywordTrue / KeywordFalse / KeywordNothing / KeywordWhile / KeywordFor / KeywordContinue / KeywordBreak / KeywordReturn
KeywordFn                  <- < 'fn' ![a-zA-Z0-9_] >
KeywordLet                 <- < 'let' ![a-zA-Z0-9_] >
KeywordPrint               <- < 'print' ![a-zA-Z0-9_] >
KeywordIf                  <- < 'if' ![a-zA-Z0-9_] >
KeywordElse                <- < 'else' ![a-zA-Z0-9_] >
KeywordWhile               <- < 'while' ![a-zA-Z0-9_] >
KeywordFor                 <- < 'for' ![a-zA-Z0-9_] >
KeywordContinue            <- < 'continue' ![a-zA-Z0-9_] >
KeywordBreak               <- < 'break' ![a-zA-Z0-9_] >
KeywordReturn              <- < 'return' ![a-zA-Z0-9_] >
KeywordTrue                <- < 'true' ![a-zA-Z0-9_] >
KeywordFalse               <- < 'false' ![a-zA-Z0-9_] >
KeywordNothing             <- < 'nothing' ![a-zA-Z0-9_] >

Block                      <- '{' Statement* '}'
Function                   <- ~KeywordFn Identifier '(' ')' Block
Statement                  <- Block / PrintStatement / LetStatement / AssignmentStatement / IfStatement / WhileStatement / ContinueStatement / BreakStatement / ReturnStatement / ForStatement / FunctionCallStatement
PrintStatement             <- ~KeywordPrint '(' Expression? ')' ';'
LetStatement               <- ~KeywordLet Identifier '=' Expression ';'
AssignmentStatement        <- Identifier '=' Expression ';'
IfStatement                <- ~KeywordIf Expression Block (~KeywordElse ~KeywordIf Expression Block)* (~KeywordElse Block)?
WhileStatement             <- ~KeywordWhile Expression Block
ForStatement               <- ~KeywordFor LetStatement Expression ';' ForUpdate Block
ForUpdate                  <- Identifier '=' Expression
ContinueStatement          <- ~KeywordContinue ';'
BreakStatement             <- ~KeywordBreak ';'
ReturnStatement            <- ~KeywordReturn Expression? ';'
FunctionCallStatement      <- FunctionCallExpression ';'

Expression                 <- InfixExpression(Atom, InfixOperator)
Atom                       <- Integer / Bool / Nothing / FunctionCallExpression / IdentifierExpression / '(' Expression ')'
IdentifierExpression       <- Identifier
FunctionCallExpression     <- Identifier '(' ')'
InfixOperator              <- < '&&' / '||' / '==' / '!=' / '<=' / '>=' / '<' / '>' / [-+/*] >
Bool                       <- ~KeywordTrue / ~KeywordFalse
Nothing                    <- ~KeywordNothing

Identifier                 <- !Keyword IdentifierToken
IdentifierToken            <- < [a-zA-Z_][a-zA-Z0-9_]* >
Integer                    <- < '-'? [0-9]+ >

InfixExpression(A, O)      <- A (O A)* {
  precedence
    L ||
    L &&
    L == !=
    L < <= > >=
    L + -
    L * /
}

End                        <- EndOfLine / EndOfFile
EndOfLine                  <- '\r\n' / '\n' / '\r'
EndOfFile                  <- !.
LineComment                <- '//' (!End .)* &End
%whitespace                <- ([ \t\r\n] / LineComment)*
```

## Semantics Notes

- `print` prints values with no automatic newline
- Division is integer division
- Reserved keywords cannot be used as identifiers
- Blocks introduce scopes
- `let` declares only in the current scope
- Variable lookup and assignment search from innermost visible scope outward
- Inner scopes may shadow outer variables
- `if`, `while`, and logical operators all use the same truthiness rules
- `return;` returns `nothing`, and `return expr;` returns the evaluated value
- Function calls used as statements must evaluate to `nothing`
- `break` exits the nearest enclosing loop
- `continue` skips the rest of the current iteration

## Project Layout

- `src/`: implementation
- `tests/pass`: programs expected to succeed
- `tests/fail`: programs expected to fail
- `scripts/`: test and debug helpers
- `third_party/peglib.h`: vendored parser dependency
- `docs/language_design.md`: target language direction
- `docs/roadmap.md`: ordered future work

## Future Direction

The intended language scope is larger than the subset above. Planned directions include richer value types, real function parameters and returns, expression statements, built-ins beyond `print`, and eventually a VM/GC/JIT path. The docs under `docs/` describe that direction; this README is the source of truth for what the code implements today.

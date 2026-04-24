# PL-JIT-Compiler-Project

Small C++23 interpreter project built around a handwritten AST, a `cpp-peglib` parser, and a direct tree-walk runtime.

## Current Language

- Multiple top-level functions
- Required zero-argument `fn main()` CLI entry point
- Function parameters and arguments
- Function calls in expression and statement position
- `return;` and `return expr;`
- Implicit `nothing` return at function end
- `print(...)` with optional expression and no automatic newline
- `println(...)` with optional expression and a trailing newline
- `let` declarations and assignment to existing variables
- Integer, boolean, and `nothing` values
- Arithmetic `+ - * /` on integers
- Relational `< <= > >=` on integers
- Equality `== !=`
- Logical `&& ||` with short-circuit behavior
- Parenthesized expressions
- Nested blocks and lexical block scoping with shadowing
- `if / else if / else`
- `while`
- `for let i = init; condition; i = update { ... }`
- `break` and `continue`
- `//` line comments

Not implemented:

- Strings, floats, arrays, tables, closures
- Unary operators or modulo
- General expression statements beyond function-call statements
- Imports/modules or type annotations

## Semantics

- Runtime values are `int64_t`, `bool`, and `nothing`
- `false`, `nothing`, and integer `0` are falsy
- Nonzero integers and `true` are truthy
- `main` must not take parameters
- `return;` returns `nothing`
- Falling off the end of a function returns `nothing`
- Function arguments are evaluated left-to-right in the caller
- Each function call creates a fresh call frame and one top-level function scope
- Parameters and top-level function locals live in that same function scope
- Equality on mismatched runtime types is `false`
- Logical operators return booleans, not original operands
- `break` and `continue` outside loops are runtime errors
- `print` renders integers as decimal, booleans as `true` / `false`, and `nothing` as `nothing`
- `println` behaves like `print` and then writes a newline
- `print()` writes nothing; `println()` writes just a newline

## Grammar

```peg
Program                    <- Function+
Function                   <- ~KeywordFn Identifier '(' Parameters ')' Block
Parameters                 <- ParameterList / EmptyParameters
ParameterList              <- Identifier (',' Identifier)*
EmptyParameters            <- ''
Block                      <- '{' Statement* '}'
Statement                  <- Block / PrintlnStatement / PrintStatement / LetStatement / AssignmentStatement / IfStatement / WhileStatement / ContinueStatement / BreakStatement / ReturnStatement / ForStatement / FunctionCallStatement
PrintStatement             <- ~KeywordPrint '(' Expression? ')' ';'
PrintlnStatement           <- ~KeywordPrintln '(' Expression? ')' ';'
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
FunctionCallExpression     <- Identifier '(' Arguments ')'
Arguments                  <- ArgumentList / EmptyArguments
ArgumentList               <- Expression (',' Expression)*
EmptyArguments             <- ''
IdentifierExpression       <- Identifier
InfixOperator              <- < '&&' / '||' / '==' / '!=' / '<=' / '>=' / '<' / '>' / [-+/*] >
Bool                       <- ~KeywordTrue / ~KeywordFalse
Nothing                    <- ~KeywordNothing
Integer                    <- < '-'? [0-9]+ >
Identifier                 <- !Keyword IdentifierToken
IdentifierToken            <- < [a-zA-Z_][a-zA-Z0-9_]* >
Keyword                    <- KeywordFn / KeywordLet / KeywordPrint / KeywordPrintln / KeywordIf / KeywordElse / KeywordTrue / KeywordFalse / KeywordNothing / KeywordWhile / KeywordFor / KeywordContinue / KeywordBreak / KeywordReturn
```

## Architecture

Pipeline:

1. `src/main.cpp` reads the source file
2. `src/parser/parser.cpp` parses it using `src/parser/language_grammar.h`
3. The parser builds the AST from `src/ast/ast.h`
4. `src/ast/ast_printer.cpp` prints normalized source for `--debug`
5. `src/tree_interpreter/tree_interpreter.cpp` executes the AST directly

Key implementation notes:

- The parser uses `cpp-peglib` semantic values stored as `std::any`
- Recursive AST expression children use `std::shared_ptr` to stay copyable for parser actions
- `PrintStatement` represents both `print` and `println` with a newline flag in the AST
- Function calls evaluate arguments in the caller, then bind parameter values in the callee
- The top-level function scope is created by function execution, not by ordinary block execution
- Statement and block execution propagate `ExecutionResult` values carrying normal flow, return, break, or continue

## Source Layout

- `src/main.cpp`: CLI entry point
- `src/parser/`: grammar and parser actions
- `src/ast/`: AST definitions and debug printer
- `src/tree_interpreter/`: runtime model and interpreter
- `src/common/overloaded.h`: `std::visit` helper
- `tests/pass`: passing golden tests
- `tests/fail`: failing golden tests
- `scripts/`: test and debug helpers

## Build

```sh
make build
```

Toolchain:

- CMake `3.20+`
- C++23
- Clang/GCC warnings: `-Wall -Wextra -Wpedantic`

## Run

```sh
./build/interpreter SOURCE
./build/interpreter --debug SOURCE
```

`--debug` prints the parsed AST before execution.

## Test

```sh
./scripts/run_single_test.sh tests/pass/<name>.ee
./scripts/run_single_test.sh tests/fail/<name>.ee
./scripts/run_passing_tests.sh
./scripts/run_failing_tests.sh
./scripts/run_all_tests.sh
./scripts/debug_test.sh tests/pass/<name>.ee
```

The suite is golden-file based:

- passing tests use `.ee` + `.out`
- failing tests use `.ee` + `.err`
- stdout/stderr text is exact-match checked
- `./scripts/run_all_tests.sh` runs the full passing and failing fixture suite

## Current Limits

- Parse and runtime failures are surfaced as `std::runtime_error` messages printed to `stderr`
- `main` parameters are rejected at runtime rather than by a separate semantic-analysis pass
- Duplicate parameter names are currently rejected at runtime when parameters are bound into scope
- `break` / `continue` placement is validated at runtime, not in a separate semantic-analysis pass
- Only top-level functions exist; there are no nested functions or closures
- Only `int64_t`, `bool`, and `nothing` exist as runtime values
- Arithmetic and relational operators currently require integer operands
- There is no VM, GC, or JIT yet
- Integer arithmetic does not currently check overflow, so signed overflow is undefined behavior instead of a reported runtime error

## Future Direction

- Add expression statements beyond function-call statements
- Add more builtins beyond `print` / `println`
- Add strings and floats
- Add unary operators and modulo
- Add arrays and tables
- Add first-class functions and closures
- Improve diagnostics and semantic validation
- Move toward a VM / GC / JIT path after interpreter semantics stabilize

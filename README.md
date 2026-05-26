# PL JIT Compiler

Small programming language project in C++23. It runs source files in the custom language by parsing them into an AST,
optionally printing the parsed form with `--debug`, and then executing them.

Current implementation:

- a parser for a custom language
- a handwritten AST
- a tree-walk runtime behind a small `ee::VM` wrapper
- a golden-test-based language implementation project

It is not a bytecode VM or JIT yet.

## ReadMe Contents

- [Build And Run](#build-and-run)
- [Scripts And Tests](#scripts-and-tests)
- [Project Layout](#project-layout)
- [Language](#language)
- [Semantics](#semantics)

## Build And Run

Requirements:

- CMake `3.20+`
- C++23 compiler

From the project root:

```sh
make build
make clean
```

Run a source file manually:

```sh
./build/interpreter SOURCE
```

Run and print the parsed AST first:

```sh
./build/interpreter --debug SOURCE
```

## Scripts And Tests

Run one passing or failing fixture:

```sh
./scripts/run_single_test.sh tests/pass/<name>.ee
./scripts/run_single_test.sh tests/fail/<name>.ee
```

Run all passing tests:

```sh
./scripts/run_passing_tests.sh
```

Run all failing tests:

```sh
./scripts/run_failing_tests.sh
```

Run the full suite:

```sh
./scripts/run_all_tests.sh
```

The tests are golden-file based:

- passing tests compare exact stdout against `.out`
- failing tests compare exact stderr against `.err`
- wording and punctuation of diagnostics matter

Scripts:

- `scripts/run_single_test.sh`: run one fixture and compare output
- `scripts/run_passing_tests.sh`: run all passing fixtures
- `scripts/run_failing_tests.sh`: run all failing fixtures
- `scripts/run_all_tests.sh`: run the full suite
- `scripts/debug_test.sh`: run one fixture with `--debug`

## Project Layout

### Top level

- `CMakeLists.txt`: build definition
- `Makefile`: small wrapper around common CMake commands
- `README.md`: project overview and usage
- `third_party/peglib.h`: vendored `cpp-peglib`

### Source

- `src/main.cpp`: CLI entry point, `--debug` handling, top-level error reporting, and `ee::VM` invocation
- `src/vm/vm.h`, `src/vm/vm.cpp`: small `ee::VM` wrapper that loads source files, stores the parsed AST, exposes debug AST output, and runs `main`
- `src/parser/language_grammar.h`: PEG grammar string
- `src/parser/parser.h`: parser interface
- `src/parser/parser.cpp`: parser construction and semantic actions that build the AST
- `src/ast/ast.h`: AST node definitions
- `src/ast/ast_printer.h`: AST printer interface
- `src/ast/ast_printer.cpp`: source-like AST pretty printer used by `--debug`
- `src/tree_interpreter/tree_interpreter.h`: interpreter entry point
- `src/tree_interpreter/runtime_value.h`, `src/tree_interpreter/runtime_value.cpp`: runtime value model and operations
- `src/tree_interpreter/runtime_state.h`, `src/tree_interpreter/runtime_state.cpp`: function table, call frames, and scopes
- `src/tree_interpreter/tree_interpreter.cpp`: AST execution and expression evaluation
- `src/common/overloaded.h`: helper for `std::visit`

### Tests

- `tests/pass`: programs expected to succeed
- `tests/fail`: programs expected to fail

For fixtures:

- `.ee`: source file
- `.out`: expected stdout for passing tests
- `.err`: expected stderr for failing tests

## Language

Current language support:

- multiple top-level functions
- required zero-argument `fn main()` entry point
- function parameters and function calls
- `print(...)` and `println(...)`
- `let` declarations with required initializers
- assignment to existing variables
- `return;` and `return expr;`
- integer, double, boolean, string, and `nothing` literals
- array literals
- identifier expressions
- array indexing and indexed assignment
- `push(array, value)` and `pop(array)`
- unary `-`, `!`
- arithmetic `+`, `-`, `*`, `/` on numeric values, with `%` on integers only
- relational `<`, `<=`, `>`, `>=` on numeric values
- equality `==`, `!=`
- logical `&&`, `||`
- parenthesized expressions
- nested blocks
- `if / else if / else`
- `while`
- `for let i = init; condition; i = update { ... }`
- `break`
- `continue`
- `//` line comments

## Semantics

- runtime values are `int64_t`, `double`, `bool`, `string`, arrays, and `nothing`
- arrays are mutable, reference-like values whose slots can hold any runtime value type
- array literal elements are evaluated eagerly from left to right
- `push` appends to an array and returns `nothing`
- `pop` removes and returns the last array element
- `false`, `nothing`, integer `0`, double `0.0`, empty strings, and empty arrays are falsy
- nonzero integers, nonzero doubles, non-empty strings, non-empty arrays, and `true` are truthy
- functions implicitly return `nothing` if no `return` executes
- `main` must exist and must not take parameters
- function arguments are evaluated left-to-right in the caller before the function is looked up and called
- each function call creates a fresh call frame
- parameters and top-level function locals live in the same function scope
- blocks use lexical scoping with shadowing
- equality on mismatched runtime types is `false`
- logical operators return booleans, not original operands
- `print` does not add a newline
- `println` prints a trailing newline
- `print()` writes nothing
- `println()` writes only a newline
- printing an array value directly is not supported
- `break` and `continue` outside loops are runtime errors
- parse and runtime failures are reported as `std::runtime_error` messages on stderr

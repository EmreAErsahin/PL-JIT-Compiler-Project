# PL-JIT Project Guidance

This repository is currently a small C++23 interpreter project built around a handwritten AST, a `cpp-peglib` parser, and a tree-walk runtime. The codebase is still in an interpreter-first phase, but it already supports more than the original arithmetic-only subset.

## Repo Summary

- Build system: CMake with a thin `Makefile` wrapper
- Main binary: `interpreter`
- Language implementation pipeline: source file -> PEG parser -> handwritten AST -> tree interpreter
- Current runtime model: dynamically typed values implemented as `std::variant<int64_t, bool, NothingValue>`
- Current program model: exactly one parsed function, executed only when its name is `main`
- Testing model: golden-file tests comparing exact stdout for passing cases and exact stderr for failing cases
- Long-term direction: larger scripting language plus VM / GC / JIT, documented in `README.md` and `docs/`

## Project Layout

- `src/main.cpp`: CLI entry point, `--debug` handling, top-level exception reporting
- `src/parse_into_ast.cpp`, `src/parse_into_ast.h`: PEG grammar and semantic actions that build the AST
- `src/pl_ast.h`: AST node definitions for expressions, statements, blocks, function, and program
- `src/tree_interpreter.cpp`, `src/tree_interpreter.h`: runtime value model, scope handling, expression evaluation, statement execution
- `src/helpers/debug_helpers.cpp`, `src/helpers/debug_helpers.h`: AST pretty-printer used by `--debug`
- `src/helpers/io_helpers.cpp`, `src/helpers/io_helpers.h`: file loading helper
- `src/helpers/overloaded.h`: `std::visit` helper
- `third_party/peglib.h`: vendored parser library
- `tests/pass`: programs expected to succeed with matching `.out`
- `tests/fail`: programs expected to fail with matching `.err`
- `scripts`: shell runners for one test, passing tests, failing tests, all tests, and debug runs
- `README.md`: best high-level source of truth for current implemented subset vs target language scope
- `docs/language_design.md`: intended language direction, not current implementation contract
- `docs/roadmap.md`: ordered backlog of major future work

## Build And Verification

- Configure and build with `make build`
- Clean with `make clean`
- Direct CMake build is also supported: `cmake -S . -B build && cmake --build build`
- Run the interpreter with `./build/interpreter SOURCE`
- Run with AST output using `./build/interpreter --debug SOURCE`
- Run one test with `./scripts/run_single_test.sh tests/pass/<name>.ee` or `tests/fail/<name>.ee`
- Run all passing tests with `./scripts/run_passing_tests.sh`
- Run all failing tests with `./scripts/run_failing_tests.sh`
- Run the full suite with `./scripts/run_all_tests.sh`
- Debug a fixture with `./scripts/debug_test.sh tests/pass/<name>.ee`

Before considering parser or runtime work complete, build and run the most relevant golden tests. Run the full suite when touching grammar, AST shape, control flow, scope handling, output formatting, or error messages.

## Current Implemented Language

The current code implements this subset today:

- One function program only
- Required `fn main()` in CLI execution
- `debugPrint(...)` with optional expression and no automatic newline
- `let` declarations with required initializer
- Assignment to an existing variable
- Integer, boolean, and `nothing` literals
- Identifier expressions
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

## Important Current Constraints

- `pl_ast::Program` stores exactly one `Function`
- The parser grammar is `Program <- Function`, not `Function*`
- Functions do not yet support parameters, return values, or calls
- There are no arrays, tables, strings, floats, closures, unary operators, modulo, or expression statements yet
- Runtime values are only `int64_t`, `bool`, and `NothingValue`
- Truthiness currently treats `false`, `nothing`, and integer `0` as falsy; nonzero integers and `true` are truthy
- Logical operators return booleans, not original operands
- Equality on mismatched runtime types evaluates to `false`
- `break` and `continue` outside loops are detected at runtime, not during parsing or semantic analysis
- Parse and runtime failures are surfaced as `std::runtime_error` messages printed to stderr by `main.cpp`
- Test fixtures depend on exact wording and punctuation of those messages

## Architecture Notes

### Parsing

- The parser is created inside `MakeParser()` in `src/parse_into_ast.cpp`
- Grammar and semantic actions live together in one file
- `cpp-peglib` semantic values use `std::any`, so AST pieces stored during parsing must be copyable
- That is why expression and block nodes use `std::shared_ptr` in specific AST locations
- Operator precedence is handled by `InfixExpression(...){ precedence ... }`

### AST

- Expressions are represented as `std::variant`
- Binary expression nodes hold shared pointers to child expressions
- Statements are also represented as `std::variant`
- Nested blocks are stored as `BlockPointer` and can appear as statements
- `IfStatement` stores explicit else-if condition/block pairs plus an optional else block
- `ForStatement` is desugared structurally in the AST into initializer, condition, update, and loop block fields

### Runtime

- The interpreter is a direct tree walk over the AST
- Scope management is stack-based via `RuntimeState::scopes_`
- `ScopeGuard` uses RAII to push/pop scopes for blocks and `for` initializer lifetime
- Variable lookup and assignment search from innermost scope outward
- Loop control flow bubbles through blocks using `ExecutionState`
- `continue` inside `for` still triggers the update step because the loop body returns `kContinue`, which is treated like non-break in the `for` handler

### Debug Output

- `--debug` prints the parsed AST back into a source-like textual form before execution
- `debug_helpers.cpp` must usually be updated when AST-visible syntax changes

## Build Toolchain

- CMake minimum version: 3.20
- Language standard: C++23 via `target_compile_features(... cxx_std_23)`
- Warnings enabled for Clang/GCC: `-Wall -Wextra -Wpedantic`
- Current executable target name: `interpreter`

## Test Conventions

- Passing tests use `.ee` input plus `.out` expected stdout
- Failing tests use `.ee` input plus `.err` expected stderr
- `scripts/run_single_test.sh` treats a pass/fail mismatch as a failed test even before diffing output
- Temporary `.tmp.out` and `.tmp.err` artifacts are cleaned automatically by the script
- Because tests are exact-output based, any user-visible text change must be intentional

## Source Of Truth Rules

- Use the code plus `README.md` as the source of truth for what is implemented today
- Use `docs/language_design.md` for intended future design only
- Use `docs/roadmap.md` for sequencing of upcoming major features
- Do not assume README target-scope items already exist just because they are documented there

## Editing Guidance

- Prefer small, local changes that fit the existing parser -> AST -> interpreter flow
- When adding a language feature, update all affected layers together:
- `src/parse_into_ast.cpp` grammar and parse actions
- `src/pl_ast.h` AST representation
- `src/tree_interpreter.cpp` runtime behavior
- `src/helpers/debug_helpers.cpp` if debug rendering should expose the feature
- `README.md` if the implemented subset changes
- tests under `tests/pass` or `tests/fail`

- Match the current style:
- free functions over heavy class hierarchies
- `std::variant` + `std::visit`
- designated initializers where they improve clarity
- targeted `std::shared_ptr` usage only where parser copyability forces it
- `std::runtime_error` with concise messages consistent with nearby code

- Do not introduce speculative VM/JIT abstractions into interpreter-stage work unless the task explicitly calls for them
- Do not edit `third_party/peglib.h` unless the task is specifically about the vendored dependency
- Do not treat generated `build/` or `cmake-build-debug/` output as source of truth

## Review Focus

When reviewing or modifying this repo, pay special attention to:

- Grammar ambiguities and keyword-boundary handling
- Exact operator precedence and associativity behavior
- Parser copyability constraints caused by `cpp-peglib` semantic values
- Block scope lifetime and shadowing rules
- `for` loop initializer lifetime and `continue` behavior
- Runtime-only validation of `break` / `continue` placement
- Exact stdout/stderr text because the tests are golden-file based

## Practical Default

Assume this repository is still in a deliberately simple interpreter-first stage. Prefer direct implementations that preserve the current architecture and make the implemented subset explicit. Keep future-language ambitions documented, but do not code as if arrays, functions, returns, closures, VM, GC, or JIT infrastructure already exist.

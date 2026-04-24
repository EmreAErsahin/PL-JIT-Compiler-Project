# PL-JIT Project Guidance

This repository is currently a small C++23 interpreter project built around a handwritten AST, a `cpp-peglib` parser, and a tree-walk runtime. The codebase is still in an interpreter-first phase, but it already supports more than the original arithmetic-only subset.

## Repo Summary

- Build system: CMake with a thin `Makefile` wrapper
- Main binary: `interpreter`
- Language implementation pipeline: source file -> PEG parser -> handwritten AST -> tree interpreter
- Current runtime model: dynamically typed values implemented as `std::variant<int64_t, bool, NothingValue>`
- Current program model: multiple top-level functions, with CLI execution starting at `main`
- Testing model: golden-file tests comparing exact stdout for passing cases and exact stderr for failing cases
- Long-term direction: larger scripting language plus VM / GC / JIT, documented in `README.md`

## Project Layout

- `src/main.cpp`: CLI entry point, `--debug` handling, top-level exception reporting
- `src/parser/parser.cpp`, `src/parser/parser.h`: parser construction and semantic actions that build the AST
- `src/parser/language_grammar.h`: PEG grammar string used by the parser
- `src/ast/ast.h`: AST node definitions for expressions, statements, blocks, functions, and programs
- `src/ast/ast_printer.cpp`, `src/ast/ast_printer.h`: AST pretty-printer used by `--debug`
- `src/tree_interpreter/tree_interpreter.cpp`, `src/tree_interpreter/tree_interpreter.h`: runtime value model, runtime state, and statement/expression execution
- `src/common/overloaded.h`: `std::visit` helper
- `third_party/peglib.h`: vendored parser library
- `tests/pass`: programs expected to succeed with matching `.out`
- `tests/fail`: programs expected to fail with matching `.err`
- `scripts`: shell runners for one test, passing tests, failing tests, all tests, and debug runs
- `README.md`: source of truth for current implementation, workflow, and future direction summary

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

- Multiple top-level function definitions
- Required `fn main()` in CLI execution
- Function parameters
- Function arguments
- `print(...)` with optional expression and no automatic newline
- `let` declarations with required initializer
- Assignment to an existing variable
- `return` with and without a value
- Integer, boolean, and `nothing` literals
- Identifier expressions
- Function calls in expression and statement position
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

- The parser grammar is `Program <- Function+`
- Functions implicitly return `nothing` when no `return` is executed
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

- The parser is created inside `MakeParser()` in `src/parser/parser.cpp`
- Grammar text lives in `src/parser/language_grammar.h` and semantic actions live in `src/parser/parser.cpp`
- `cpp-peglib` semantic values use `std::any`, so AST pieces stored during parsing must be copyable
- That is why expression and block nodes use `std::shared_ptr` in specific AST locations
- Parameter and argument list rules normalize empty and non-empty lists before parent actions consume them
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
- Runtime state and AST execution are split between `RuntimeState` and `Interpreter` inside `src/tree_interpreter/tree_interpreter.cpp`
- Scope management is stack-based via `RuntimeState` call frames and lexical scope stacks
- `ScopeGuard` uses RAII to push/pop scopes for blocks, function top-level scope, and `for` initializer lifetime
- Variable lookup and assignment search from innermost scope outward
- Function calls evaluate arguments in the caller, then bind parameter values in the callee's top-level scope
- Loop and function control flow bubbles through blocks using `ExecutionResult`
- `continue` inside `for` still triggers the update step before the next condition check

### Debug Output

- `--debug` prints the parsed AST back into a source-like textual form before execution
- `ast_printer.cpp` must usually be updated when AST-visible syntax changes

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
- Keep future direction notes in `README.md` concise and clearly separated from implemented behavior

## Editing Guidance

- Prefer small, local changes that fit the existing parser -> AST -> interpreter flow
- When adding a language feature, update all affected layers together:
- `src/parser/language_grammar.h` grammar text when syntax changes
- `src/parser/parser.cpp` parse actions
- `src/ast/ast.h` AST representation
- `src/tree_interpreter/tree_interpreter.cpp` runtime behavior
- `src/ast/ast_printer.cpp` if debug rendering should expose the feature
- `README.md` if the implemented subset changes
- tests under `tests/pass` or `tests/fail`

- Match the current style:
- small focused classes where state ownership is clearer than flat free functions
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

Assume this repository is still in a deliberately simple interpreter-first stage. Prefer direct implementations that preserve the current architecture and make the implemented subset explicit. Keep future-language ambitions documented, but do not code as if arrays, closures, VM, GC, or JIT infrastructure already exist.

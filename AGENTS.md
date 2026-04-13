# PL-JIT Project Guidance

This repository is a small C++23 interpreter project built around a handwritten AST, a PEG parser, and a tree-walk runtime. Keep changes minimal, local, and aligned with the current stage of the project.

## Project Shape

- Entry point: `src/main.cpp`
- Parser and grammar: `src/parse_into_ast.cpp`, `src/parse_into_ast.h`
- AST definitions: `src/pl_ast.h`
- Runtime interpreter: `src/tree_interpreter.cpp`, `src/tree_interpreter.h`
- Debug AST printer: `src/helpers/debug_helpers.cpp`, `src/helpers/debug_helpers.h`
- File IO helpers: `src/helpers/io_helpers.cpp`, `src/helpers/io_helpers.h`
- Third-party dependency: `third_party/peglib.h`
- Golden tests: `tests/pass`, `tests/fail`
- Test runners: `scripts/run_single_test.sh`, `scripts/run_passing_tests.sh`, `scripts/run_failing_tests.sh`, `scripts/run_all_tests.sh`, `scripts/debug_test.sh`

## Build And Verification

- Configure and build with `make build`
- Clean with `make clean`
- Run the interpreter with `./build/interpreter SOURCE`
- Run with AST debug output using `./build/interpreter --debug SOURCE` or `./scripts/debug_test.sh tests/pass/<name>.ee`
- Run one golden test with `./scripts/run_single_test.sh tests/pass/<name>.ee` or a failing test in `tests/fail`
- Run the full suite with `./scripts/run_all_tests.sh`
- CI currently runs `make clean && make build` followed by `./scripts/run_all_tests.sh`

Before considering work done, build the project and run the most relevant golden tests. Run the full suite when changes affect parsing, AST shape, execution semantics, CLI behavior, or error messages.

## Language And Runtime Status

The currently implemented language supports:

- A single function program
- A required `main` entry point
- `debugPrint(...)`
- `let` declarations with required initializers
- Assignment to existing variables
- Integer, boolean, and `nothing` literals
- Identifier expressions
- Arithmetic `+`, `-`, `*`, `/` on integers only
- Parenthesized expressions

Notable current constraints:

- `Program` currently stores exactly one function in `pl_ast::Program`
- The interpreter enforces the existence of `main` in `tree_interpreter.cpp`
- Arithmetic on non-integers is a runtime error
- Division by zero is a runtime error
- Parse and runtime failures are surfaced as `std::runtime_error` messages written to stderr
- Passing tests assert stdout exactly via `.out`; failing tests assert stderr exactly via `.err`

## Editing Guidance

- Prefer small changes within the existing parser -> AST -> interpreter pipeline
- Update all affected layers together when adding a language feature:
- Grammar and semantic actions in `src/parse_into_ast.cpp`
- AST types in `src/pl_ast.h`
- Execution semantics in `src/tree_interpreter.cpp`
- Debug rendering in `src/helpers/debug_helpers.cpp` when AST output should reflect the change
- README grammar and semantics when user-visible language behavior changes
- Tests under `tests/pass` or `tests/fail`

- Follow the current style:
- C++23
- namespaces per subsystem
- free functions over heavy class hierarchies
- `std::variant` plus `std::visit` for AST and runtime values
- designated initializers are already used and are acceptable here
- use `std::shared_ptr` in AST expression nodes only where current parser-copyability constraints require it
- error out with clear `std::runtime_error` messages consistent with nearby code

- Do not introduce large abstractions, visitors, or multi-file refactors unless the task clearly requires them
- Do not edit `third_party/peglib.h` unless the task explicitly targets vendored dependencies
- Do not treat `build/` contents as source of truth; it is generated output

## Testing Guidance

- Keep test fixtures in sync with behavior changes
- New passing behavior should usually add a `tests/pass/<name>.ee` and matching `.out`
- New failure behavior should usually add a `tests/fail/<name>.ee` and matching `.err`
- Because tests compare exact output, preserve formatting and punctuation intentionally
- If you change an error message, update the corresponding `.err` fixtures deliberately rather than incidentally

## Review Focus

When reviewing or implementing changes in this repo, pay special attention to:

- Grammar ambiguities and keyword boundary issues
- AST ownership and copyability constraints caused by `cpp-peglib` semantic values using `std::any`
- Regressions in operator precedence and parenthesized expression handling
- Variable declaration versus assignment semantics
- Exact stdout and stderr behavior, since tests are golden-file based

## Practical Default

Assume this repository is still in an interpreter-first stage. Prefer direct, understandable implementations that preserve the current architecture over speculative JIT, GC, or multi-function designs unless the task explicitly asks for those larger changes.

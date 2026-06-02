# PL JIT Compiler

A small C++23 programming language project. It parses `.ee` source files into an AST and executes them with a tree-walk
interpreter exposed through `ee::VM`.

Despite the name, this is not a JIT yet.

## Requirements

- CMake 3.20+
- C++23 compiler
- Make

## Build And Run

Build the CLI interpreter:

```sh
make cli
```

Run a program:

```sh
./build/interpreter/interpreter path/to/file.ee
```

Print the parsed AST before running:

```sh
./build/interpreter/interpreter --debug path/to/file.ee
```

Useful Make targets:

- `make cli`: build the command-line interpreter
- `make runtime`: build only the reusable `ee_runtime` library target
- `make test`: build and run the full test suite
- `make clean`: remove `build/`

## Tests

Run everything:

```sh
make test
```

Run already-built tests directly:

```sh
./scripts/run_all_tests.sh
```

The suite has two kinds of golden-file tests:

- language tests run `.ee` files through the CLI interpreter
- embedding tests run C++ executables linked against `ee_runtime`

Passing tests compare stdout with `.out`. Failing tests compare stderr with `.err`.

Useful scripts:

- `scripts/language_single.sh <test.ee>`
- `scripts/language_passing.sh`
- `scripts/language_failing.sh`
- `scripts/language_debug.sh <test.ee>`
- `scripts/embedding_single.sh <test.cpp>`
- `scripts/embedding_passing.sh`
- `scripts/embedding_failing.sh`

## Project Map

```text
CMakeLists.txt        build targets and embedding-test discovery
Makefile              common build/test commands
scripts/              golden-test runners
src/main.cpp          CLI entry point
src/vm/               ee::VM wrapper
src/parser/           grammar and parser
src/ast/              AST and debug printer
src/tree_interpreter/ runtime values, state, and execution
tests/language_tests/ CLI language fixtures
tests/embedding_tests/ C++ embedding fixtures
third_party/          vendored dependencies
```

Main CMake targets:

- `ee_runtime`: reusable runtime library
- `interpreter`: CLI executable linked against `ee_runtime`
- `embedding_tests`: builds all embedding test executables

## Language Snapshot

Currently supported:

- functions, parameters, calls, `return`, and CLI `fn main()`
- `let` declarations, assignment, blocks, and lexical scoping
- integers, doubles, booleans, strings, arrays, and `nothing`
- `print`, `println`, arithmetic, comparison, equality, and logical operators
- `if`, `else if`, `else`, `while`, `for`, `break`, and `continue`
- array indexing, indexed assignment, `push`, and `pop`
- `//` line comments

Runtime notes:

- `false`, `nothing`, numeric zero, empty strings, and empty arrays are falsy
- arrays are mutable reference-like values with heterogeneous slots
- parse and runtime failures are reported as `std::runtime_error` messages

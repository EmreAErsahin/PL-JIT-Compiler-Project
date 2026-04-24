# Roadmap

This roadmap is ordered from the current interpreter baseline in the codebase, not from the larger aspirational feature list alone.

## Near Term

- Tighten parse and runtime error messages while preserving exact tested output where intended
- Add semantic analysis for control-flow misuse and other static checks that are currently runtime-only
- Improve CLI and debug ergonomics around parse failures and AST inspection
- Keep `README.md` and tests aligned whenever user-visible behavior changes

## Language Growth On Top Of The Current Core

- Add expression statements beyond zero-argument function call statements
- Add function parameters and arguments
- Add more built-ins beyond `print`, likely starting with `println` and `type()`
- Extend the runtime value model with strings and floats
- Add unary operators and modulo
- Add arrays and tables
- Add first-class functions and closures

## Runtime And Tooling

- Improve parser and interpreter diagnostics with clearer source locations
- Add more systematic semantic validation before execution
- Revisit AST ownership choices if parser constraints change
- Expand test coverage as new syntax and runtime types land

## Longer Term

- Define a bytecode format and bytecode VM
- Move heap-backed runtime types behind a clearer runtime object model
- Add garbage collection once heap types justify it
- Explore JIT compilation after the VM and runtime semantics stabilize

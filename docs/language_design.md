# Language Design

This document describes the target language, not just the subset implemented today.

## Goal

Build an embeddable scripting language for C++ host applications.

The rough direction is “Lua-like use case, different syntax and ergonomics.” The language should stay small enough to understand completely and simple enough to embed cleanly.

## Main Differences From Lua

- `fn` instead of `function`
- `nothing` instead of `nil`
- `!=` instead of `~=`
- braces define blocks
- `//` comments only
- locals introduced explicitly with `let`
- 0-based indexing is the likely direction

## Core Design Decisions

- Dynamically typed
- No type annotations
- Braces-based syntax
- Newlines are whitespace, not syntax
- Script and native functions should eventually use the same call syntax
- CLI mode should require `main`; embedded mode should not

## Planned Value Model

Early/core types:

- integers
- booleans
- `nothing`

Later candidates:

- strings
- floats
- arrays / vector-like collections
- objects / tables

## Planned Semantics

- Integer division while only integers exist
- Blocks introduce lexical scopes

Current implemented expression semantics:

- Arithmetic operators require integer operands
- Relational operators require integer operands and produce booleans
- Equality operators are valid across all current runtime value types
- Logical operators short-circuit and produce booleans
- `if` conditions use the same truthiness rules as logical operators
- Current truthiness rule:
  - falsy: `false`, `nothing`, `0`
  - truthy: `true`, nonzero integers

Current implemented control flow:

- `if`
- `else if`
- `else`

## Embedding Direction

Long-term, native and script functions should share one dispatch model.

Target shape:

```cpp
VM vm;

vm.register("openDoor", [](Args a) -> Value {
    game.open_door(a[0].as_int());
    return Value::nothing();
});

vm.load("game_logic.script");
vm.call("main");
vm.call("onPlayerEnter", {player_id});
```

## Target PEG

This is a target grammar shape, not the currently implemented one.

```peg
Program        <- Function*
Function       <- 'fn' Identifier '(' ParamList ')' Block
ParamList      <- (Identifier (',' Identifier)*)?
Block          <- '{' Statement* '}'

Statement      <- LetStmt
               / AssignStmt
               / ReturnStmt
               / IfStmt
               / WhileStmt
               / ExprStmt

LetStmt        <- 'let' Identifier '=' Expr ';'
AssignStmt     <- Identifier '=' Expr ';'
ReturnStmt     <- 'return' Expr? ';'
IfStmt         <- 'if' Expr Block ('else' (IfStmt / Block))?
WhileStmt      <- 'while' Expr Block
ExprStmt       <- Expr ';'

Expr           <- LogicalOr
LogicalOr      <- LogicalAnd ('||' LogicalAnd)*
LogicalAnd     <- Equality ('&&' Equality)*
Equality       <- Comparison (('==' / '!=') Comparison)*
Comparison     <- Addition (('<=' / '>=' / '<' / '>') Addition)*
Addition       <- Multiplication (('+' / '-') Multiplication)*
Multiplication <- Unary (('*' / '/' / '%') Unary)*
Unary          <- ('-' / '!') Unary / Call
Call           <- Primary ('(' ArgList ')')*
ArgList        <- (Expr (',' Expr)*)?
Primary        <- Integer / Boolean / Nothing / Identifier / '(' Expr ')'

Boolean        <- 'true' / 'false'
Nothing        <- 'nothing'
Integer        <- < '-'? [0-9]+ >
Identifier     <- !Keyword < [a-zA-Z_][a-zA-Z0-9_]* >
Keyword        <- 'fn' / 'let' / 'return' / 'if' / 'else'
               / 'while' / 'true' / 'false' / 'nothing'
Comment        <- '//' (!'\n' .)* ('\n' / !.)
%whitespace    <- ([ \t\r\n] / Comment)*
```

## Scope Of This Document

Use this file for intended language direction.

Use `README.md` for what the code actually implements right now.

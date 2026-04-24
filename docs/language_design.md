# Language Design

This document describes the target language direction, not just the subset implemented today.

`README.md` is the source of truth for current behavior. Use this file for planned semantics and longer-term design intent.

## Goal

Build a small embeddable scripting language for C++ host applications.

The rough direction is Lua-like in use case, but with different syntax and clearer explicitness around scoping and declarations.

## Current Baseline

The existing implementation already establishes these design constraints:

- Dynamically typed runtime
- Braces-based blocks
- Newlines are whitespace, not syntax
- Explicit local declarations with `let`
- Multiple top-level functions
- CLI execution starting from `fn main()`
- Lexical block scoping with shadowing
- Tree-walk execution over a handwritten AST

The current runtime value model is still intentionally small:

- integers
- booleans
- `nothing`

## Main Differences From Lua

- `fn` instead of `function`
- `nothing` instead of `nil`
- `!=` instead of `~=`
- braces define blocks
- `//` comments only
- locals introduced explicitly with `let`
- 0-based indexing is the planned direction for arrays

## Design Direction

The next language tiers are intended to grow from the current interpreter baseline without changing the broad shape of the language:

- Keep a small core syntax
- Preserve explicit local declaration with `let`
- Keep script syntax usable both from the CLI and in embedded hosts
- Make native and script functions converge on the same call model over time
- Preserve lexical scoping rules as the language grows

## Planned Value Model

Planned core types:

- integers
- floats
- booleans
- `nothing`
- strings
- arrays
- tables
- closures

## Planned Core Features

This is the intended target scope for the current project phase, not a statement that all of it exists yet.

- `let` declarations with required initializer
- Assignment to variables
- Assignment to array index
- Assignment to table bracket access
- Assignment to table dot access
- `if` / `else if` / `else`
- `while`
- C-style numeric `for`
- `break`
- `continue`
- `return` with and without value
- Multiple function definitions
- Function parameters
- Function arguments
- Call expressions
- First-class functions
- Closures with captured environment
- Arithmetic `+`, `-`, `*`, `/`, `%`
- Mixed int/float arithmetic with float promotion
- Relational `<`, `>`, `<=`, `>=`
- Equality `==`, `!=`
- Logical `&&`, `||` with short-circuit evaluation
- Unary minus
- Logical not
- String concatenation `..`
- Length operator `#`
- Array literals
- Table literals
- Array indexing
- Table dot access
- Table bracket access
- Lexical block scoping with shadowing
- `print` with multiple arguments
- `print` with no automatic newline as the minimal builtin output primitive
- Expression statements
- Line comments
- Shared heap-backed runtime objects where appropriate
- Built-in `type()`

## Explicit Non-Goals For Now

These are not currently part of the intended near-term surface area:

- String interpolation
- String methods
- Lambdas as a separate syntax from named functions
- Try/catch/throw
- Block comments
- Match/switch
- For-in / iterator protocol
- Multiple return values
- Multiple assignment
- Varargs
- Destructuring
- Ternary expression
- Do-while / repeat-until
- Type annotations
- Standard library modules
- Module system / imports
- Operator overloading / metatables
- Coroutines
- Native function registration API stability guarantees yet
- Negative indexing
- Implicit `tostring` / `tonumber` conversions
- A tracing GC before the runtime grows beyond the current interpreter stage

## Semantics Direction

Planned semantic principles:

- Blocks introduce lexical scopes
- Function calls should eventually support values in and values out, while preserving the current scope model
- Native and script functions should eventually share one dispatch model
- Embedded mode should not require a `main` entry point even though CLI mode does today
- Simpler, explicit rules are preferred over magical coercions

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

## Target Grammar Shape

This is a target grammar sketch, not the currently implemented grammar.

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
               / BreakStmt
               / ContinueStmt
               / ForStmt
               / ExprStmt

LetStmt        <- 'let' Identifier '=' Expr ';'
AssignStmt     <- Identifier '=' Expr ';'
ReturnStmt     <- 'return' Expr? ';'
IfStmt         <- 'if' Expr Block ('else' (IfStmt / Block))?
WhileStmt      <- 'while' Expr Block
BreakStmt      <- 'break' ';'
ContinueStmt   <- 'continue' ';'
ForStmt        <- 'for' '(' Expr? ';' Expr? ';' Expr? ')' Block
ExprStmt       <- Expr ';'

Expr           <- LogicalOr
LogicalOr      <- LogicalAnd ('||' LogicalAnd)*
LogicalAnd     <- Equality ('&&' Equality)*
Equality       <- Comparison (('==' / '!=') Comparison)*
Comparison     <- Addition (('<=' / '>=' / '<' / '>') Addition)*
Addition       <- Multiplication (('+' / '-') Multiplication)*
Multiplication <- Concat (('*' / '/' / '%') Concat)*
Concat         <- Unary ('..' Unary)*
Unary          <- ('-' / '!' / '#') Unary / Call
Call           <- Primary (('(' ArgList ')') / ('[' Expr ']') / ('.' Identifier))*
ArgList        <- (Expr (',' Expr)*)?
Primary        <- Integer / Float / Boolean / Nothing / String / ArrayLiteral / TableLiteral / Identifier / '(' Expr ')'

Boolean        <- 'true' / 'false'
Nothing        <- 'nothing'
Integer        <- < '-'? [0-9]+ >
Float          <- < '-'? [0-9]+ '.' [0-9]+ >
String         <- '"' ... '"'
ArrayLiteral   <- '[' (Expr (',' Expr)*)? ']'
TableLiteral   <- '{' (Identifier ':' Expr (',' Identifier ':' Expr)*)? '}'
Identifier     <- !Keyword < [a-zA-Z_][a-zA-Z0-9_]* >
Keyword        <- 'fn' / 'let' / 'return' / 'if' / 'else'
               / 'while' / 'for' / 'break' / 'continue'
               / 'true' / 'false' / 'nothing'
Comment        <- '//' (!'\n' .)* ('\n' / !.)
%whitespace    <- ([ \t\r\n] / Comment)*
```

## Scope Of This Document

Use this file for intended language direction.

Use `README.md` for what the code actually implements right now.

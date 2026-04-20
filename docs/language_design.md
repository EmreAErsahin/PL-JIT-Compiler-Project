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

Planned core types:

- integers
- floats
- booleans
- `nothing`
- strings
- arrays
- tables
- closures

## Planned Semantics

- Integer division while only integers exist
- Blocks introduce lexical scopes

Target language scope for the current project phase:

### Included

- Integer type
- Float type
- Boolean type
- Nothing type
- String type with `\n`, `\t`, `\\`, `\"` escape sequences
- Array type with 0-based indexing
- Table type
- Closure type
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
- `print` with multiple args
- `debugPrint`
- Expression statements
- Line comments
- `shared_ptr` for strings, arrays, tables, and closures
- Built-in `type()`

### Not Included

- String interpolation
- String methods
- Lambdas
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
- Native function registration API
- Negative indexing
- `tostring` / `tonumber` conversions
- Real tracing GC in the VM tier

Current implemented subset today is still smaller than that full target scope. Use `README.md` as the source of truth for what the code currently supports.

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

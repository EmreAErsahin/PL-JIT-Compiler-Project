# PL-JIT-Compiler-Project

Small C++23 interpreter project for a custom language. The current implementation uses a handwritten AST, a `cpp-peglib` parser, and a tree-walk interpreter.

## Current Features

- One function program
- Required `fn main()` in CLI mode
- `debugPrint(...)`
- `let` declarations with required initializers
- Assignment to existing variables
- Integer, boolean, and `nothing` literals
- Identifier expressions
- Arithmetic `+`, `-`, `*`, `/` on integers only
- Relational `<`, `>`, `<=`, `>=` on integers only
- Equality `==`, `!=` across all current runtime value types
- Logical `&&`, `||` with short-circuit evaluation
- `if`, `else if`, `else`
- `while`
- `break`
- `continue`
- Parenthesized expressions
- Nested `{ ... }` blocks
- Lexical block scopes with shadowing
- `//` line comments

## Current Limitations

- `Program` stores exactly one function
- No `return`, params, or function calls
- Arithmetic on non-integers is a runtime error
- Relational operators on non-integers are a runtime error
- Division by zero is a runtime error
- Errors are surfaced as `std::runtime_error` messages written to stderr

## Build And Run

```sh
make build
./build/interpreter SOURCE
./build/interpreter --debug SOURCE
make clean
```

## Tests

```sh
./scripts/run_single_test.sh tests/pass/<name>.ee
./scripts/run_single_test.sh tests/fail/<name>.ee
./scripts/run_passing_tests.sh
./scripts/run_failing_tests.sh
./scripts/run_all_tests.sh
./scripts/debug_test.sh tests/pass/<name>.ee
```

CI runs:

```sh
make clean && make build
./scripts/run_all_tests.sh
```

## Current Grammar

```peg
Program                    <- Function
KeywordFn                  <- < 'fn' ![a-zA-Z0-9_] >
KeywordLet                 <- < 'let' ![a-zA-Z0-9_] >
KeywordDebugPrint          <- < 'debugPrint' ![a-zA-Z0-9_] >
KeywordIf                  <- < 'if' ![a-zA-Z0-9_] >
KeywordElse                <- < 'else' ![a-zA-Z0-9_] >
KeywordWhile               <- < 'while' ![a-zA-Z0-9_] >
KeywordContinue            <- < 'continue' ![a-zA-Z0-9_] >
KeywordBreak               <- < 'break' ![a-zA-Z0-9_] >
KeywordTrue                <- < 'true' ![a-zA-Z0-9_] >
KeywordFalse               <- < 'false' ![a-zA-Z0-9_] >
KeywordNothing             <- < 'nothing' ![a-zA-Z0-9_] >

Block                      <- '{' Statement* '}'
Function                   <- ~KeywordFn Identifier '(' ')' Block
Statement                  <- Block / DebugPrintStatement / LetStatement / AssignmentStatement / IfStatement / WhileStatement / ContinueStatement / BreakStatement
DebugPrintStatement        <- ~KeywordDebugPrint '(' Expression? ')' ';'
LetStatement               <- ~KeywordLet Identifier '=' Expression ';'
AssignmentStatement        <- Identifier '=' Expression ';'
IfStatement                <- ~KeywordIf Expression Block (~KeywordElse ~KeywordIf Expression Block)* (~KeywordElse Block)?
WhileStatement             <- ~KeywordWhile Expression Block
ContinueStatement          <- ~KeywordContinue ';'
BreakStatement             <- ~KeywordBreak ';'

Expression                 <- InfixExpression(Atom, InfixOperator)
Atom                       <- Integer / Bool / Nothing / IdentifierExpression / '(' Expression ')'
IdentifierExpression       <- Identifier
InfixOperator              <- < '&&' / '||' / '==' / '!=' / '<=' / '>=' / '<' / '>' / [-+/*] >
Bool                       <- ~KeywordTrue / ~KeywordFalse
Nothing                    <- ~KeywordNothing

Identifier                 <- !KeywordFn !KeywordLet !KeywordDebugPrint !KeywordIf !KeywordElse !KeywordWhile !KeywordContinue !KeywordBreak !KeywordTrue !KeywordFalse !KeywordNothing IdentifierToken
IdentifierToken            <- < [a-zA-Z_][a-zA-Z0-9_]* >
Integer                    <- < '-'? [0-9]+ >

InfixExpression(A, O)      <- A (O A)* {
  precedence
    L ||
    L &&
    L == !=
    L < <= > >=
    L + -
    L * /
}

End                        <- EndOfLine / EndOfFile
EndOfLine                  <- '\r\n' / '\n' / '\r'
EndOfFile                  <- !.
LineComment                <- '//' (!End .)* &End
%whitespace                <- ([ \t\r\n] / LineComment)*
```

## Semantics Notes

- `debugPrint` prints with no automatic newline
- Division is integer division
- Reserved keywords cannot be used as identifiers: `fn`, `let`, `debugPrint`, `if`, `else`, `while`, `continue`, `break`, `true`, `false`, `nothing`
- Blocks introduce scopes
- `if` conditions use the same truthiness rules as logical operators
- `let` declares in the current scope only
- Variable lookup and assignment search from innermost scope outward
- Inner scopes may shadow outer variables
- `==` and `!=` are valid across all current runtime value types; cross-type equality evaluates to `false`
- `&&` and `||` short-circuit and return booleans
- Logical truthiness currently treats `false`, `nothing`, and `0` as falsy; nonzero integers and `true` are truthy
- `if` / `else if` / `else` execute the first truthy branch only
- `while` uses the same truthiness rules as `if` and logical operators
- `break` exits the nearest enclosing loop
- `continue` skips the rest of the current loop iteration and reevaluates the loop condition
- `break` and `continue` outside a loop are runtime errors

## Project Layout

- `src/main.cpp`: CLI entry point
- `src/parse_into_ast.cpp`: grammar and parser actions
- `src/pl_ast.h`: AST definitions
- `src/tree_interpreter.cpp`: runtime interpreter
- `src/helpers/debug_helpers.cpp`: AST debug printer
- `tests/pass`, `tests/fail`: golden tests
- `scripts`: test/debug scripts

## Docs

- `docs/language_design.md`: future language direction
- `docs/roadmap.md`: feature roadmap

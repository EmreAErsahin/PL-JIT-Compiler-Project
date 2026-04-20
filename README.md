# PL-JIT-Compiler-Project

Small C++23 language implementation project built around a handwritten AST, a `cpp-peglib` parser, and a tree-walk interpreter. The repository is still in an interpreter-first stage, but the target language scope is larger than the currently implemented subset.

## Target Language Scope

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
- Assignment to array index: `arr[0] = 5`
- Assignment to table bracket access: `t["key"] = 5`
- Assignment to table dot access: `t.key = 5`
- `if` / `else if` / `else`
- `while`
- C-style numeric `for`
- `break`
- `continue`
- `return`
- Multiple function definitions
- Function parameters
- Function arguments
- Call expressions: `foo(1, 2)`
- First-class functions
- Closures with captured environment
- Arithmetic: `+`, `-`, `*`, `/`, `%`
- Mixed int/float arithmetic with float promotion
- Relational: `<`, `>`, `<=`, `>=`
- Equality: `==`, `!=`
- Logical `&&`, `||` with short-circuit evaluation
- Unary minus: `-x`
- Logical not: `!x`
- String concatenation: `..`
- Length operator: `#`
- Array literals: `[1, 2, 3]`
- Table literals: `{name: "emre", age: 21}`
- Array indexing: `arr[0]`
- Table dot access: `t.name`
- Table bracket access: `t["name"]`
- Lexical block scoping with shadowing
- `print` with multiple arguments
- `debugPrint`
- Expression statements
- Line comments: `//`
- `shared_ptr` for strings, arrays, tables, and closures
- Built-in `type()` returning the runtime type as a string

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

## Current Implemented Subset

The code in this repository currently implements a smaller interpreter-first subset:

- One function program with required `fn main()` in CLI mode
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
- C-style numeric `for`
- `break`
- `continue`
- Parenthesized expressions
- Nested `{ ... }` blocks
- Lexical block scopes with shadowing
- `//` line comments

Current implementation limits worth keeping in mind:

- `Program` currently stores exactly one function
- No `return`, params, function calls, arrays, tables, strings, floats, or closures yet
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
Keyword                    <- KeywordFn / KeywordLet / KeywordDebugPrint / KeywordIf / KeywordElse / KeywordTrue / KeywordFalse / KeywordNothing / KeywordWhile / KeywordFor / KeywordContinue / KeywordBreak
KeywordFn                  <- < 'fn' ![a-zA-Z0-9_] >
KeywordLet                 <- < 'let' ![a-zA-Z0-9_] >
KeywordDebugPrint          <- < 'debugPrint' ![a-zA-Z0-9_] >
KeywordIf                  <- < 'if' ![a-zA-Z0-9_] >
KeywordElse                <- < 'else' ![a-zA-Z0-9_] >
KeywordWhile               <- < 'while' ![a-zA-Z0-9_] >
KeywordFor                 <- < 'for' ![a-zA-Z0-9_] >
KeywordContinue            <- < 'continue' ![a-zA-Z0-9_] >
KeywordBreak               <- < 'break' ![a-zA-Z0-9_] >
KeywordTrue                <- < 'true' ![a-zA-Z0-9_] >
KeywordFalse               <- < 'false' ![a-zA-Z0-9_] >
KeywordNothing             <- < 'nothing' ![a-zA-Z0-9_] >

Block                      <- '{' Statement* '}'
Function                   <- ~KeywordFn Identifier '(' ')' Block
Statement                  <- Block / DebugPrintStatement / LetStatement / AssignmentStatement / IfStatement / WhileStatement / ContinueStatement / BreakStatement / ForStatement
DebugPrintStatement        <- ~KeywordDebugPrint '(' Expression? ')' ';'
LetStatement               <- ~KeywordLet Identifier '=' Expression ';'
AssignmentStatement        <- Identifier '=' Expression ';'
IfStatement                <- ~KeywordIf Expression Block (~KeywordElse ~KeywordIf Expression Block)* (~KeywordElse Block)?
WhileStatement             <- ~KeywordWhile Expression Block
ForStatement               <- ~KeywordFor LetStatement Expression ';' ForUpdate Block
ForUpdate                  <- Identifier '=' Expression
ContinueStatement          <- ~KeywordContinue ';'
BreakStatement             <- ~KeywordBreak ';'

Expression                 <- InfixExpression(Atom, InfixOperator)
Atom                       <- Integer / Bool / Nothing / IdentifierExpression / '(' Expression ')'
IdentifierExpression       <- Identifier
InfixOperator              <- < '&&' / '||' / '==' / '!=' / '<=' / '>=' / '<' / '>' / [-+/*] >
Bool                       <- ~KeywordTrue / ~KeywordFalse
Nothing                    <- ~KeywordNothing

Identifier                 <- !Keyword IdentifierToken
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
- Reserved keywords cannot be used as identifiers: `fn`, `let`, `debugPrint`, `if`, `else`, `while`, `for`, `continue`, `break`, `true`, `false`, `nothing`
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
- `for` uses the shape `for let i = init; condition; i = update { ... }`
- The `for` initializer variable lives for the full loop and is not visible after the loop ends
- `break` exits the nearest enclosing loop
- `continue` skips the rest of the current loop iteration and reevaluates the loop condition
- In `for`, `continue` still runs the update step before reevaluating the loop condition
- `break` and `continue` outside a loop are runtime errors

## Future Direction

The target language scope above is the intended near-to-medium-term direction for the project. As those features land, update the parser, AST, interpreter, debug printer, tests, and docs together so the implemented subset stays explicit.

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

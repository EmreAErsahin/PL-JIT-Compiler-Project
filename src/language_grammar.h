#ifndef language_grammar_H
#define language_grammar_H

#include <string_view>

namespace parse_into_ast_grammar {
  // Grammar notes:
  // - <...>: treat the whole match as one token (tokens have automatic whitespace consumption after)
  // - ~Rule: parse it, but ignore its semantic value
  // - !Rule: next input must not match Rule
  // - %whitespace: automatically skips between tokens, literal string, & start of input
  // - precedence/InfixExpressions/Atoms is the built-in functionality to follow PEMDAS
  inline constexpr std::string_view kPegGrammar = R"(
      Program                    <- Function+
      Keyword                    <- KeywordFn / KeywordLet / KeywordDebugPrint / KeywordIf / KeywordElse / KeywordTrue / KeywordFalse / KeywordNothing / KeywordWhile / KeywordFor / KeywordContinue / KeywordBreak
      KeywordFn                  <- < 'fn' ![a-zA-Z0-9_] >
      KeywordLet                 <- < 'let' ![a-zA-Z0-9_] >
      KeywordDebugPrint          <- < 'debugPrint' ![a-zA-Z0-9_] >
      KeywordIf                  <- < 'if' ![a-zA-Z0-9_] >
      KeywordElse                <- < 'else' ![a-zA-Z0-9_] >
      KeywordTrue                <- < 'true' ![a-zA-Z0-9_] >
      KeywordFalse               <- < 'false' ![a-zA-Z0-9_] >
      KeywordNothing             <- < 'nothing' ![a-zA-Z0-9_] >
      KeywordWhile               <- < 'while' ![a-zA-Z0-9_] >
      KeywordFor                 <- < 'for' ![a-zA-Z0-9_] >
      KeywordContinue            <- < 'continue' ![a-zA-Z0-9_] >
      KeywordBreak               <- < 'break' ![a-zA-Z0-9_] >
      Block                      <- '{' Statement* '}'
      Function                   <- ~KeywordFn Identifier '(' ')' Block
      Statement                  <- Block / DebugPrintStatement / LetStatement / AssignmentStatement / IfStatement / WhileStatement / ContinueStatement / BreakStatement / ForStatement / FunctionCallStatement
      DebugPrintStatement        <- ~KeywordDebugPrint '(' Expression? ')' ';'
      LetStatement               <- ~KeywordLet Identifier '=' Expression ';'
      AssignmentStatement        <- Identifier '=' Expression ';'
      IfStatement                <- ~KeywordIf Expression Block (~KeywordElse ~KeywordIf Expression Block)* (~KeywordElse Block)?
      WhileStatement             <- ~KeywordWhile Expression Block
      ForStatement               <- ~KeywordFor LetStatement Expression ';' ForUpdate Block
      ForUpdate                  <- Identifier '=' Expression
      ContinueStatement          <- ~KeywordContinue ';'
      BreakStatement             <- ~KeywordBreak ';'
      FunctionCallStatement      <- FunctionCallExpression ';'
      Expression                 <- InfixExpression(Atom, InfixOperator)
      Atom                       <- Integer / Bool / Nothing / FunctionCallExpression / IdentifierExpression / '(' Expression ')'
      IdentifierExpression       <- Identifier
      FunctionCallExpression     <- Identifier '(' ')'
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
      End <- EndOfLine / EndOfFile
      EndOfLine <- '\r\n' / '\n' / '\r'
      EndOfFile                <-  !.
      LineComment <- '//' (!End .)* &End
      %whitespace                <- ([ \t\r\n] / LineComment)*
    )";
} // namespace parse_into_ast_grammar

#endif // language_grammar_H

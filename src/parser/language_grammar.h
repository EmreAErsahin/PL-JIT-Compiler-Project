#ifndef language_grammar_H
#define language_grammar_H

#include <string_view>

namespace parser_grammar {
  // Grammar notes:
  // - <...>: treat the whole match as one token (tokens have automatic whitespace consumption after)
  // - ~Rule: parse it, but ignore its semantic value
  // - !Rule: next input must not match Rule
  // - %whitespace: automatically skips between tokens, literal string, & start of input
  // - InfixExpression(...){ precedence ... } is cpp-peglib's built-in expression precedence helper
  inline constexpr std::string_view kPegGrammar = R"(
      Program                    <- Function+

      Function                   <- ~KeywordFn Identifier '(' Parameters ')' Block
      Parameters                 <- ParameterList / EmptyParameters
      ParameterList              <- Identifier (',' Identifier)*
      EmptyParameters            <- ''

      Block                      <- '{' Statement* '}'

      Statement                  <- Block / PrintlnStatement / PrintStatement / LetStatement / IndexAssignmentStatement / AssignmentStatement / IfStatement / WhileStatement / ContinueStatement / BreakStatement / ReturnStatement / ForStatement / FunctionCallStatement
      PrintStatement             <- ~KeywordPrint '(' Expression? ')' ';'
      PrintlnStatement           <- ~KeywordPrintln '(' Expression? ')' ';'
      LetStatement               <- ~KeywordLet Identifier '=' Expression ';'
      IndexAssignmentStatement   <- IndexExpression '=' Expression ';'
      AssignmentStatement        <- Identifier '=' Expression ';'
      FunctionCallStatement      <- FunctionCallExpression ';'
      ReturnStatement            <- ~KeywordReturn Expression? ';'
      BreakStatement             <- ~KeywordBreak ';'
      ContinueStatement          <- ~KeywordContinue ';'
      IfStatement                <- ~KeywordIf Expression Block (~KeywordElse ~KeywordIf Expression Block)* (~KeywordElse Block)?
      WhileStatement             <- ~KeywordWhile Expression Block
      ForStatement               <- ~KeywordFor LetStatement Expression ';' ForUpdate Block
      ForUpdate                  <- Identifier '=' Expression

      Expression                 <- InfixExpression(UnaryExpression, InfixOperator)
      InfixOperator              <- < '&&' / '||' / '==' / '!=' / '<=' / '>=' / '<' / '>' / [-+/*%] >
      InfixExpression(A, O)      <- A (O A)* {
        precedence
          L ||
          L &&
          L == !=
          L < <= > >=
          L + -
          L * / %
      }

      UnaryExpression            <- UnaryOperator UnaryExpression / IndexExpression / Atom
      UnaryOperator              <- < '-' / '!' >

      IndexExpression            <- Atom IndexSuffix+
      IndexSuffix                <- '[' Expression ']'

      Atom                       <- Double / Integer / Bool / String / Array / Nothing / FunctionCallExpression / IdentifierExpression / '(' Expression ')'
      Integer                    <- < [0-9]+ >
      Double                     <- < [0-9]* '.' [0-9]+ >
      Bool                       <- ~KeywordTrue / ~KeywordFalse
      String                     <- < '"' ('\\' [nt\\] / !('"' / '\\' / EndOfLine) .)* '"' >
      Array                      <- '[' Arguments ']'
      Nothing                    <- ~KeywordNothing

      FunctionCallExpression     <- Identifier '(' Arguments ')'
      Arguments                  <- ArgumentList / EmptyArguments
      ArgumentList               <- Expression (',' Expression)*
      EmptyArguments             <- ''

      IdentifierExpression       <- Identifier
      Identifier                 <- !Keyword IdentifierToken
      IdentifierToken            <- < [a-zA-Z_][a-zA-Z0-9_]* >

      Keyword                    <- KeywordFn / KeywordLet / KeywordPrint / KeywordPrintln / KeywordIf / KeywordElse / KeywordTrue / KeywordFalse / KeywordNothing / KeywordWhile / KeywordFor / KeywordContinue / KeywordBreak / KeywordReturn
      KeywordFn                  <- < 'fn' ![a-zA-Z0-9_] >
      KeywordLet                 <- < 'let' ![a-zA-Z0-9_] >
      KeywordPrint               <- < 'print' ![a-zA-Z0-9_] >
      KeywordPrintln             <- < 'println' ![a-zA-Z0-9_] >
      KeywordIf                  <- < 'if' ![a-zA-Z0-9_] >
      KeywordElse                <- < 'else' ![a-zA-Z0-9_] >
      KeywordTrue                <- < 'true' ![a-zA-Z0-9_] >
      KeywordFalse               <- < 'false' ![a-zA-Z0-9_] >
      KeywordNothing             <- < 'nothing' ![a-zA-Z0-9_] >
      KeywordWhile               <- < 'while' ![a-zA-Z0-9_] >
      KeywordFor                 <- < 'for' ![a-zA-Z0-9_] >
      KeywordContinue            <- < 'continue' ![a-zA-Z0-9_] >
      KeywordBreak               <- < 'break' ![a-zA-Z0-9_] >
      KeywordReturn              <- < 'return' ![a-zA-Z0-9_] >

      End                        <- EndOfLine / EndOfFile
      EndOfLine                  <- '\r\n' / '\n' / '\r'
      EndOfFile                  <- !.
      LineComment                <- '//' (!End .)* &End
      %whitespace                <- ([ \t\r\n] / LineComment)*
    )";
} // namespace parser_grammar

#endif // language_grammar_H

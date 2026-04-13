#include "peglib.h"

#include <concepts>
#include <any>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "parse_into_ast.h"

namespace parse_into_ast {
  // Parsing library requires copyable types
  template <std::copy_constructible T>
  T CastSemanticValueTo(const peg::SemanticValues& semantic_values, const size_t index) {
    return std::any_cast<T>(semantic_values[index]);
  }

  peg::parser MakeParser() {
    peg::parser parser;

    constexpr auto pl_grammar = R"(
      Program                    <- Function
      KeywordFn                  <- < 'fn' ![a-zA-Z0-9_] >
      KeywordLet                 <- < 'let' ![a-zA-Z0-9_] >
      KeywordDebugPrint          <- < 'debugPrint' ![a-zA-Z0-9_] >
      KeywordTrue                <- < 'true' ![a-zA-Z0-9_] >
      KeywordFalse               <- < 'false' ![a-zA-Z0-9_] >
      KeywordNothing             <- < 'nothing' ![a-zA-Z0-9_] >
      Function                   <- KeywordFn Identifier '(' ')' '{' Statement* '}'
      Statement                  <- DebugPrintStatement / LetStatement / AssignmentStatement
      DebugPrintStatement        <- KeywordDebugPrint '(' Expression? ')' ';'
      LetStatement               <- KeywordLet Identifier '=' Expression ';'
      AssignmentStatement        <- Identifier '=' Expression ';'
      Expression                 <- InfixExpression(Atom, ArithmeticOperator)
      Atom                       <- Integer / Bool / Nothing / IdentifierExpression / '(' Expression ')'
      IdentifierExpression       <- Identifier
      ArithmeticOperator         <- < [-+/*] >
      Bool                       <- KeywordTrue / KeywordFalse
      Nothing                    <- KeywordNothing
      Identifier                 <- !KeywordFn !KeywordLet !KeywordDebugPrint !KeywordTrue !KeywordFalse !KeywordNothing IdentifierToken
      IdentifierToken            <- < [a-zA-Z_][a-zA-Z0-9_]* >
      Integer                    <- < '-'? [0-9]+ >
      InfixExpression(A, O)      <- A (O A)* {
        precedence
          L + -
          L * /
      }
      End <- EndOfLine / EndOfFile
      EndOfLine <- '\r\n' / '\n' / '\r'
      EndOfFile                <-  !.
      LineComment <- '//' (!End .)* &End
      %whitespace                <- ([ \t\r\n] / LineComment)*
    )";

    if (!parser.load_grammar(pl_grammar)) {
      throw std::runtime_error("MakeParser: failed to load parser grammar");
    }

    parser["IdentifierToken"] = [](const peg::SemanticValues& semantic_values) {
      return pl_ast::Identifier{semantic_values.token_to_string()};
    };

    parser["Identifier"] = [](const peg::SemanticValues& semantic_values) {
      return CastSemanticValueTo<pl_ast::Identifier>(semantic_values, 0);
    };

    parser["Integer"] = [](const peg::SemanticValues& semantic_values) {
      return pl_ast::ExpressionVariant{pl_ast::IntegerLiteralExpression{semantic_values.token_to_number<int64_t>()}};
    };

    parser["Bool"] = [](const peg::SemanticValues& semantic_values) {
      return pl_ast::ExpressionVariant{pl_ast::BoolLiteralExpression{semantic_values.token_to_string() == "true"}};
    };

    parser["Nothing"] = [](const peg::SemanticValues&) { return pl_ast::ExpressionVariant{pl_ast::NothingLiteralExpression{}}; };

    parser["ArithmeticOperator"] = [](const peg::SemanticValues& semantic_values) {
      switch (*semantic_values.sv().begin()) {
        case '+': return pl_ast::ArithmeticOperator::ADD;
        case '-': return pl_ast::ArithmeticOperator::SUBTRACT;
        case '*': return pl_ast::ArithmeticOperator::MULTIPLY;
        case '/': return pl_ast::ArithmeticOperator::DIVIDE;
        default: throw std::runtime_error("ArithmeticOperator: unsupported operator");
      }
    };

    parser["IdentifierExpression"] = [](const peg::SemanticValues& semantic_values) {
      return pl_ast::ExpressionVariant{pl_ast::IdentifierExpression{CastSemanticValueTo<pl_ast::Identifier>(semantic_values, 0)}};
    };

    parser["Atom"] = [](const peg::SemanticValues& semantic_values) {
      return CastSemanticValueTo<pl_ast::ExpressionVariant>(semantic_values, 0);
    };

    parser["Expression"] = [](const peg::SemanticValues& semantic_values) {
      return CastSemanticValueTo<pl_ast::ExpressionVariant>(semantic_values, 0);
    };

    parser["InfixExpression"] = [](const peg::SemanticValues& semantic_values) {
      auto result = CastSemanticValueTo<pl_ast::ExpressionVariant>(semantic_values, 0);

      if (semantic_values.size() > 1) {
        result = pl_ast::ExpressionVariant{
          pl_ast::BinaryExpression{
                                   .left_operand_ = std::make_shared<pl_ast::ExpressionVariant>(std::move(result)),
                                   .operator_ = CastSemanticValueTo<pl_ast::ArithmeticOperator>(semantic_values, 1),
                                   .right_operand_ =
              std::make_shared<pl_ast::ExpressionVariant>(CastSemanticValueTo<pl_ast::ExpressionVariant>(semantic_values, 2)),
                                   }
        };
      }

      return result;
    };

    parser["DebugPrintStatement"] = [](const peg::SemanticValues& semantic_values) {
      pl_ast::DebugPrintStatement debug_print_statement;
      if (semantic_values.size() > 1) {
        debug_print_statement.expression_ = CastSemanticValueTo<pl_ast::ExpressionVariant>(semantic_values, 1);
      }
      return pl_ast::StatementVariant{debug_print_statement};
    };

    parser["LetStatement"] = [](const peg::SemanticValues& semantic_values) {
      return pl_ast::StatementVariant{
        pl_ast::LetStatement{
                             .identifier_ = CastSemanticValueTo<pl_ast::Identifier>(semantic_values, 1),
                             .initializer_expression_ = CastSemanticValueTo<pl_ast::ExpressionVariant>(semantic_values, 2),
                             }
      };
    };

    parser["AssignmentStatement"] = [](const peg::SemanticValues& semantic_values) {
      return pl_ast::StatementVariant{
        pl_ast::AssignmentStatement{
                                    .identifier_ = CastSemanticValueTo<pl_ast::Identifier>(semantic_values, 0),
                                    .assigned_expression_ = CastSemanticValueTo<pl_ast::ExpressionVariant>(semantic_values, 1),
                                    }
      };
    };

    parser["Statement"] = [](const peg::SemanticValues& semantic_values) {
      return CastSemanticValueTo<pl_ast::StatementVariant>(semantic_values, 0);
    };

    // Using designated initializers for clarity & safety against changing field positions
    parser["Function"] = [](const peg::SemanticValues& semantic_values) {
      std::vector<pl_ast::StatementVariant> statement_variants;
      statement_variants.reserve(semantic_values.size() - 2);

      for (size_t semantic_value_index = 2; semantic_value_index < semantic_values.size(); ++semantic_value_index) {
        statement_variants.push_back(CastSemanticValueTo<pl_ast::StatementVariant>(semantic_values, semantic_value_index));
      }

      return pl_ast::Function{
        .identifier_ = CastSemanticValueTo<pl_ast::Identifier>(semantic_values, 1),
        .statements_ = std::move(statement_variants),
      };
    };

    parser["Program"] = [](const peg::SemanticValues& semantic_values) {
      return pl_ast::Program{.function_ = CastSemanticValueTo<pl_ast::Function>(semantic_values, 0)};
    };

    // Trades memory usage for better speed with memoization of grammar rules
    parser.enable_packrat_parsing();

    return parser;
  }

  pl_ast::Program ParseFileContentsIntoAST(const std::string& file_contents) {
    auto parser = MakeParser();

    size_t error_line = 0;
    size_t error_column = 0;
    std::string error_message;

    parser.set_logger([&error_line, &error_column, &error_message](const size_t line, const size_t column, const std::string& message) {
      error_line = line;
      error_column = column;
      error_message = message;
    });

    pl_ast::Program program_ast;
    if (!parser.parse(file_contents, program_ast)) {
      // TODO: Sometimes logger won't populate all of these, potentially leading to poor error messages
      throw std::runtime_error(
        "ParseFileContentsIntoAST: parse failed at " + std::to_string(error_line) + ':' + std::to_string(error_column) + ": "
        + error_message
      );
    }

    return program_ast;
  }
} // namespace parse_into_ast

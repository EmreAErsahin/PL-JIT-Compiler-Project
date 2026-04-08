#include "peglib.h"

#include <any>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "parse_into_ast.h"

namespace parse_into_ast {
  peg::parser MakeParser() {
    peg::parser parser;

    constexpr auto pl_grammar = R"(
      Program                    <- Function
      Function                   <- 'fn' Identifier '(' ')' '{' Statement* '}'
      Statement                  <- DebugPrint ';'
      DebugPrint                 <- 'debugPrint' '(' Expression? ')'
      Expression                 <- InfixExpression(Atom, ArithmeticOperator)
      Atom                       <- Integer / Bool / Nothing / '(' Expression ')'
      ArithmeticOperator         <- < [-+/*] >
      Bool                       <- 'true' / 'false'
      Nothing                    <- 'nothing'
      Identifier                 <- < [a-zA-Z_][a-zA-Z0-9_]* >
      Integer                    <- < '-'? [0-9]+ >
      InfixExpression(A, O)      <- A (O A)* {
        precedence
          L + -
          L * /
      }
      %whitespace                <- [ \t\r\n]*
    )";

    if (!parser.load_grammar(pl_grammar)) {
      throw std::runtime_error("MakeParser: failed to load parser grammar");
    }

    parser["Identifier"] = [](const peg::SemanticValues& semantic_values) { return pl_ast::Identifier{semantic_values.token_to_string()}; };

    parser["Integer"] = [](const peg::SemanticValues& semantic_values) {
      return pl_ast::ExpressionVariant{pl_ast::IntegerLiteralExpression{semantic_values.token_to_number<int64_t>()}};
    };

    parser["ArithmeticOperator"] = [](const peg::SemanticValues& semantic_values) {
      switch (*semantic_values.sv().begin()) {
        case '+': return pl_ast::ArithmeticOperator::ADD;
        case '-': return pl_ast::ArithmeticOperator::SUBTRACT;
        case '*': return pl_ast::ArithmeticOperator::MULTIPLY;
        case '/': return pl_ast::ArithmeticOperator::DIVIDE;
        default: throw std::runtime_error("ArithmeticOperator: unsupported operator");
      }
    };

    parser["Bool"] = [](const peg::SemanticValues& semantic_values) {
      return pl_ast::ExpressionVariant{pl_ast::BoolLiteralExpression{semantic_values.token_to_string() == "true"}};
    };

    parser["Nothing"] = [](const peg::SemanticValues&) { return pl_ast::ExpressionVariant{pl_ast::NothingLiteralExpression{}}; };

    parser["Atom"] = [](const peg::SemanticValues& semantic_values) {
      return std::any_cast<pl_ast::ExpressionVariant>(semantic_values[0]);
    };

    parser["Expression"] = [](const peg::SemanticValues& semantic_values) {
      return std::any_cast<pl_ast::ExpressionVariant>(semantic_values[0]);
    };

    parser["InfixExpression"] = [](const peg::SemanticValues& semantic_values) {
      auto result = std::any_cast<pl_ast::ExpressionVariant>(semantic_values[0]);

      if (semantic_values.size() > 1) {
        result = pl_ast::ExpressionVariant{
          pl_ast::BinaryExpression{
                                   .left_operand_ = std::make_shared<pl_ast::ExpressionVariant>(std::move(result)),
                                   .operator_ = std::any_cast<pl_ast::ArithmeticOperator>(semantic_values[1]),
                                   .right_operand_ = std::make_shared<pl_ast::ExpressionVariant>(std::any_cast<pl_ast::ExpressionVariant>(semantic_values[2])),
                                   }
        };
      }

      return result;
    };

    parser["DebugPrint"] = [](const peg::SemanticValues& semantic_values) {
      pl_ast::DebugPrintStatement debug_print_statement;
      if (!semantic_values.empty()) {
        debug_print_statement.expression_ = std::any_cast<pl_ast::ExpressionVariant>(semantic_values[0]);
      }
      return debug_print_statement;
    };

    parser["Statement"] = [](const peg::SemanticValues& semantic_values) {
      return pl_ast::StatementVariant{std::any_cast<pl_ast::DebugPrintStatement>(semantic_values[0])};
    };

    // Using designated initializers for clarity & safety against changing field positions
    parser["Function"] = [](const peg::SemanticValues& semantic_values) {
      std::vector<pl_ast::StatementVariant> statement_variants;
      statement_variants.reserve(semantic_values.size() - 1);

      for (size_t semantic_value_index = 1; semantic_value_index < semantic_values.size(); ++semantic_value_index) {
        statement_variants.push_back(std::any_cast<pl_ast::StatementVariant>(semantic_values[semantic_value_index]));
      }

      return pl_ast::Function{
        .identifier_ = std::any_cast<pl_ast::Identifier>(semantic_values[0]),
        .statements_ = std::move(statement_variants),
      };
    };

    parser["Program"] = [](const peg::SemanticValues& semantic_values) {
      return pl_ast::Program{.function_ = std::any_cast<pl_ast::Function>(semantic_values[0])};
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
      // TODO: Sometimes logger won't populate all of these
      throw std::runtime_error(
        "ParseFileContentsIntoAST: parse failed at " + std::to_string(error_line) + ':' + std::to_string(error_column) + ": "
        + error_message
      );
    }

    return program_ast;
  }
} // namespace parse_into_ast

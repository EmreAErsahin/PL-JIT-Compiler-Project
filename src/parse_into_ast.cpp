#include "peglib.h"

#include <concepts>
#include <any>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "helpers/overloaded.h"
#include "parse_into_ast.h"

namespace parse_into_ast {
  using InfixOperator =
    std::variant<pl_ast::ArithmeticOperator, pl_ast::RelationalOperator, pl_ast::EqualityOperator, pl_ast::LogicalOperator>;

  // Parsing library requires copyable types
  template <std::copy_constructible T>
  T CastSemanticValueTo(const peg::SemanticValues& semantic_values, const size_t index) {
    return std::any_cast<T>(semantic_values[index]);
  }

  template <typename ExpressionNode, typename Operator>
  requires requires(pl_ast::ExpressionPointer left_operand, Operator operator_value, pl_ast::ExpressionPointer right_operand) {
    ExpressionNode{
      .left_operand_ = left_operand,
      .operator_ = operator_value,
      .right_operand_ = right_operand,
    };
  }
  pl_ast::ExpressionVariant CreateTypedInfixExpression(
    pl_ast::ExpressionVariant left_operand, const Operator operator_value, pl_ast::ExpressionVariant right_operand
  ) {
    return pl_ast::ExpressionVariant{
      ExpressionNode{
                     .left_operand_ = std::make_shared<pl_ast::ExpressionVariant>(std::move(left_operand)),
                     .operator_ = operator_value,
                     .right_operand_ = std::make_shared<pl_ast::ExpressionVariant>(std::move(right_operand)),
                     }
    };
  }

  pl_ast::ExpressionVariant CreateInfixExpression(
    pl_ast::ExpressionVariant left_operand, const InfixOperator& infix_operator, pl_ast::ExpressionVariant right_operand
  ) {
    return std::visit(
      template_helpers::Overloaded{
        [&left_operand, &right_operand](const pl_ast::ArithmeticOperator operator_value) {
          return CreateTypedInfixExpression<pl_ast::ArithmeticExpression>(
            std::move(left_operand), operator_value, std::move(right_operand)
          );
        },
        [&left_operand, &right_operand](const pl_ast::RelationalOperator operator_value) {
          return CreateTypedInfixExpression<pl_ast::RelationalExpression>(
            std::move(left_operand), operator_value, std::move(right_operand)
          );
        },
        [&left_operand, &right_operand](const pl_ast::EqualityOperator operator_value) {
          return CreateTypedInfixExpression<pl_ast::EqualityExpression>(std::move(left_operand), operator_value, std::move(right_operand));
        },
        [&left_operand, &right_operand](const pl_ast::LogicalOperator operator_value) {
          return CreateTypedInfixExpression<pl_ast::LogicalExpression>(std::move(left_operand), operator_value, std::move(right_operand));
        },
      },
      infix_operator
    );
  }

  peg::parser MakeParser() {
    peg::parser parser;

    // Grammar notes:
    // - <...>: treat the whole match as one token (tokens have automatic whitespace consumption after)
    // - ~Rule: parse it, but ignore its semantic value
    // - !Rule: next input must not match Rule
    // - %whitespace: automatically skips between tokens, literal string, & start of input
    // - precedence/InfixExpressions/Atoms is the built-in functionality to follow PEMDAS
    constexpr auto pl_grammar = R"(
      Program                    <- Function
      KeywordFn                  <- < 'fn' ![a-zA-Z0-9_] >
      KeywordLet                 <- < 'let' ![a-zA-Z0-9_] >
      KeywordDebugPrint          <- < 'debugPrint' ![a-zA-Z0-9_] >
      KeywordTrue                <- < 'true' ![a-zA-Z0-9_] >
      KeywordFalse               <- < 'false' ![a-zA-Z0-9_] >
      KeywordNothing             <- < 'nothing' ![a-zA-Z0-9_] >
      Block <- '{' Statement* '}'
      Function                   <- ~KeywordFn Identifier '(' ')' Block
      Statement                  <- Block / DebugPrintStatement / LetStatement / AssignmentStatement
      DebugPrintStatement        <- ~KeywordDebugPrint '(' Expression? ')' ';'
      LetStatement               <- ~KeywordLet Identifier '=' Expression ';'
      AssignmentStatement        <- Identifier '=' Expression ';'
      Expression                 <- InfixExpression(Atom, InfixOperator)
      Atom                       <- Integer / Bool / Nothing / IdentifierExpression / '(' Expression ')'
      IdentifierExpression       <- Identifier
      InfixOperator              <- < '&&' / '||' / '==' / '!=' / '<=' / '>=' / '<' / '>' / [-+/*] >
      Bool                       <- ~KeywordTrue / ~KeywordFalse
      Nothing                    <- ~KeywordNothing
      Identifier                 <- !KeywordFn !KeywordLet !KeywordDebugPrint !KeywordTrue !KeywordFalse !KeywordNothing IdentifierToken
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

    if (!parser.load_grammar(pl_grammar)) {
      throw std::runtime_error("MakeParser: failed to load parser grammar");
    }

    parser["IdentifierToken"] = [](const peg::SemanticValues& semantic_values) {
      return pl_ast::Identifier{semantic_values.token_to_string()};
    };

    parser["Integer"] = [](const peg::SemanticValues& semantic_values) {
      return pl_ast::ExpressionVariant{pl_ast::IntegerLiteralExpression{semantic_values.token_to_number<int64_t>()}};
    };

    parser["Bool"] = [](const peg::SemanticValues& semantic_values) {
      return pl_ast::ExpressionVariant{pl_ast::BoolLiteralExpression{semantic_values.choice() == 0}};
    };

    parser["Nothing"] = [](const peg::SemanticValues&) { return pl_ast::ExpressionVariant{pl_ast::NothingLiteralExpression{}}; };

    parser["InfixOperator"] = [](const peg::SemanticValues& semantic_values) {
      const auto operator_token = semantic_values.token_to_string();
      if (operator_token == "+") {
        return InfixOperator{pl_ast::ArithmeticOperator::kAdd};
      } else if (operator_token == "-") {
        return InfixOperator{pl_ast::ArithmeticOperator::kSubtract};
      } else if (operator_token == "*") {
        return InfixOperator{pl_ast::ArithmeticOperator::kMultiply};
      } else if (operator_token == "/") {
        return InfixOperator{pl_ast::ArithmeticOperator::kDivide};
      } else if (operator_token == "<") {
        return InfixOperator{pl_ast::RelationalOperator::kLessThan};
      } else if (operator_token == "<=") {
        return InfixOperator{pl_ast::RelationalOperator::kLessThanOrEqual};
      } else if (operator_token == ">") {
        return InfixOperator{pl_ast::RelationalOperator::kGreaterThan};
      } else if (operator_token == ">=") {
        return InfixOperator{pl_ast::RelationalOperator::kGreaterThanOrEqual};
      } else if (operator_token == "==") {
        return InfixOperator{pl_ast::EqualityOperator::kEqual};
      } else if (operator_token == "!=") {
        return InfixOperator{pl_ast::EqualityOperator::kNotEqual};
      } else if (operator_token == "&&") {
        return InfixOperator{pl_ast::LogicalOperator::kAnd};
      } else if (operator_token == "||") {
        return InfixOperator{pl_ast::LogicalOperator::kOr};
      }
      throw std::runtime_error("InfixOperator: unsupported operator");
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
        return CreateInfixExpression(
          std::move(result), CastSemanticValueTo<InfixOperator>(semantic_values, 1),
          CastSemanticValueTo<pl_ast::ExpressionVariant>(semantic_values, 2)
        );
      }

      return result;
    };

    parser["DebugPrintStatement"] = [](const peg::SemanticValues& semantic_values) {
      pl_ast::DebugPrintStatement debug_print_statement;
      if (!semantic_values.empty()) {
        debug_print_statement.expression_ = CastSemanticValueTo<pl_ast::ExpressionVariant>(semantic_values, 0);
      }
      return pl_ast::StatementVariant{debug_print_statement};
    };

    parser["LetStatement"] = [](const peg::SemanticValues& semantic_values) {
      return pl_ast::StatementVariant{
        pl_ast::LetStatement{
                             .identifier_ = CastSemanticValueTo<pl_ast::Identifier>(semantic_values, 0),
                             .initializer_expression_ = CastSemanticValueTo<pl_ast::ExpressionVariant>(semantic_values, 1),
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
      // Checking if we have a block pointer (needs special cast bc we store it as a block ptr bc it may need to be put into a function)
      // TODO: Don't love the use of .choice this is brittle. Changing peg can break this action
      if (semantic_values.choice() == 0) {
        return pl_ast::StatementVariant{CastSemanticValueTo<pl_ast::BlockPointer>(semantic_values, 0)};
      }
      return CastSemanticValueTo<pl_ast::StatementVariant>(semantic_values, 0);
    };

    parser["Block"] = [](const peg::SemanticValues& semantic_values) {
      std::vector<pl_ast::StatementVariant> statement_variants;
      statement_variants.reserve(semantic_values.size());

      for (size_t semantic_value_index = 0; semantic_value_index < semantic_values.size(); ++semantic_value_index) {
        statement_variants.push_back(CastSemanticValueTo<pl_ast::StatementVariant>(semantic_values, semantic_value_index));
      }

      return std::make_shared<pl_ast::Block>(pl_ast::Block{.statements_ = std::move(statement_variants)});
    };

    // Using designated initializers for clarity & safety against changing field positions
    parser["Function"] = [](const peg::SemanticValues& semantic_values) {
      return pl_ast::Function{
        .identifier_ = CastSemanticValueTo<pl_ast::Identifier>(semantic_values, 0),
        .function_block_ = CastSemanticValueTo<pl_ast::BlockPointer>(semantic_values, 1),
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

#include "peglib.h"

#include <concepts>
#include <any>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "../common/overloaded.h"
#include "language_grammar.h"
#include "parser.h"

namespace parser {
  using InfixOperator =
    std::variant<pl_ast::ArithmeticOperator, pl_ast::RelationalOperator, pl_ast::EqualityOperator, pl_ast::LogicalOperator>;

  // Parsing library requires copyable types
  template <std::copy_constructible T>
  T CastSemanticValueTo(const peg::SemanticValues& semantic_values, const size_t index) {
    return std::any_cast<T>(semantic_values[index]);
  }

  template <typename ExpressionNode, typename Operator>
  pl_ast::ExpressionVariant
  MakeBinaryExpression(pl_ast::ExpressionVariant left_operand, const Operator operator_value, pl_ast::ExpressionVariant right_operand) {
    return pl_ast::ExpressionVariant{
      ExpressionNode{
                     .left_operand_ = std::make_shared<pl_ast::ExpressionVariant>(std::move(left_operand)),
                     .operator_ = operator_value,
                     .right_operand_ = std::make_shared<pl_ast::ExpressionVariant>(std::move(right_operand)),
                     }
    };
  }

  peg::parser MakeParser() {
    peg::parser parser;

    if (!parser.load_grammar(std::string(parser_grammar::kPegGrammar))) {
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

    parser["FunctionCallExpression"] = [](const peg::SemanticValues& semantic_values) {
      return pl_ast::ExpressionVariant{
        pl_ast::FunctionCallExpression{.function_name_ = CastSemanticValueTo<pl_ast::Identifier>(semantic_values, 0)}
      };
    };

    parser["Atom"] = [](const peg::SemanticValues& semantic_values) {
      return CastSemanticValueTo<pl_ast::ExpressionVariant>(semantic_values, 0);
    };

    parser["Expression"] = [](const peg::SemanticValues& semantic_values) {
      return CastSemanticValueTo<pl_ast::ExpressionVariant>(semantic_values, 0);
    };

    parser["InfixExpression"] = [](const peg::SemanticValues& semantic_values) {
      auto left_operand = CastSemanticValueTo<pl_ast::ExpressionVariant>(semantic_values, 0);

      if (semantic_values.size() == 1) {
        return left_operand;
      }

      const auto infix_operator = CastSemanticValueTo<InfixOperator>(semantic_values, 1);
      auto right_operand = CastSemanticValueTo<pl_ast::ExpressionVariant>(semantic_values, 2);

      return std::visit(
        template_helpers::Overloaded{
          [&left_operand, &right_operand](const pl_ast::ArithmeticOperator operator_value) -> pl_ast::ExpressionVariant {
            return MakeBinaryExpression<pl_ast::ArithmeticExpression>(std::move(left_operand), operator_value, std::move(right_operand));
          },
          [&left_operand, &right_operand](const pl_ast::RelationalOperator operator_value) -> pl_ast::ExpressionVariant {
            return MakeBinaryExpression<pl_ast::RelationalExpression>(std::move(left_operand), operator_value, std::move(right_operand));
          },
          [&left_operand, &right_operand](const pl_ast::EqualityOperator operator_value) -> pl_ast::ExpressionVariant {
            return MakeBinaryExpression<pl_ast::EqualityExpression>(std::move(left_operand), operator_value, std::move(right_operand));
          },
          [&left_operand, &right_operand](const pl_ast::LogicalOperator operator_value) -> pl_ast::ExpressionVariant {
            return MakeBinaryExpression<pl_ast::LogicalExpression>(std::move(left_operand), operator_value, std::move(right_operand));
          },
        },
        infix_operator
      );
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

    parser["IfStatement"] = [](const peg::SemanticValues& semantic_values) {
      // First two semantic values are if, last one is else if it's present, everything in the middle is else ifs
      auto if_condition = CastSemanticValueTo<pl_ast::ExpressionVariant>(semantic_values, 0);
      auto if_block = std::get<pl_ast::BlockPointer>(CastSemanticValueTo<pl_ast::StatementVariant>(semantic_values, 1));

      pl_ast::ElseIfConditionBlockPairs else_if_branches;
      // Semantic values are: if condition, if block, zero or more else-if condition/block pairs,
      // and an optional trailing else block.
      const size_t number_of_else_if_branches = (semantic_values.size() - 2) / 2;
      for (size_t current_branch = 1; current_branch <= number_of_else_if_branches; ++current_branch) {
        else_if_branches.emplace_back(
          CastSemanticValueTo<pl_ast::ExpressionVariant>(semantic_values, current_branch * 2),
          std::get<pl_ast::BlockPointer>(CastSemanticValueTo<pl_ast::StatementVariant>(semantic_values, current_branch * 2 + 1))
        );
      }

      std::optional<pl_ast::BlockPointer> else_block;
      if (semantic_values.size() % 2) {
        else_block =
          std::get<pl_ast::BlockPointer>(CastSemanticValueTo<pl_ast::StatementVariant>(semantic_values, semantic_values.size() - 1));
      }

      return pl_ast::StatementVariant{
        pl_ast::IfStatement{
                            .if_condition_ = std::move(if_condition),
                            .if_block_ = std::move(if_block),
                            .else_if_branches_ = std::move(else_if_branches),
                            .else_block_ = std::move(else_block),
                            }
      };
    };

    parser["WhileStatement"] = [](const peg::SemanticValues& semantic_values) {
      auto while_condition = CastSemanticValueTo<pl_ast::ExpressionVariant>(semantic_values, 0);
      auto while_block = std::get<pl_ast::BlockPointer>(CastSemanticValueTo<pl_ast::StatementVariant>(semantic_values, 1));

      return pl_ast::StatementVariant{
        pl_ast::WhileStatement{.while_condition_ = std::move(while_condition), .while_block_ = std::move(while_block)}
      };
    };

    parser["ForStatement"] = [](const peg::SemanticValues& semantic_values) {
      // Statements are all held as statement variant, so we cant any cast straight to let statement / assignment statement
      auto initializer = std::get<pl_ast::LetStatement>(CastSemanticValueTo<pl_ast::StatementVariant>(semantic_values, 0));
      auto condition = CastSemanticValueTo<pl_ast::ExpressionVariant>(semantic_values, 1);
      auto update = std::get<pl_ast::AssignmentStatement>(CastSemanticValueTo<pl_ast::StatementVariant>(semantic_values, 2));
      auto for_block = std::get<pl_ast::BlockPointer>(CastSemanticValueTo<pl_ast::StatementVariant>(semantic_values, 3));

      return pl_ast::StatementVariant{
        pl_ast::ForStatement{
                             .initializer_ = std::move(initializer),
                             .condition_ = std::move(condition),
                             .update_ = std::move(update),
                             .for_block_ = std::move(for_block)
        }
      };
    };

    parser["ForUpdate"] = [](const peg::SemanticValues& semantic_values) {
      return pl_ast::StatementVariant{
        pl_ast::AssignmentStatement{
                                    .identifier_ = CastSemanticValueTo<pl_ast::Identifier>(semantic_values, 0),
                                    .assigned_expression_ = CastSemanticValueTo<pl_ast::ExpressionVariant>(semantic_values, 1),
                                    }
      };
    };

    parser["ContinueStatement"] = [](const peg::SemanticValues&) { return pl_ast::StatementVariant{pl_ast::ContinueStatement{}}; };

    parser["BreakStatement"] = [](const peg::SemanticValues&) { return pl_ast::StatementVariant{pl_ast::BreakStatement{}}; };

    parser["FunctionCallStatement"] = [](const peg::SemanticValues& semantic_values) {
      return pl_ast::StatementVariant{pl_ast::FunctionCallStatement{
        .function_call_ = std::get<pl_ast::FunctionCallExpression>(CastSemanticValueTo<pl_ast::ExpressionVariant>(semantic_values, 0))
      }};
    };

    parser["Statement"] = [](const peg::SemanticValues& semantic_values) {
      return CastSemanticValueTo<pl_ast::StatementVariant>(semantic_values, 0);
    };

    parser["Block"] = [](const peg::SemanticValues& semantic_values) {
      std::vector<pl_ast::StatementVariant> statement_variants;
      statement_variants.reserve(semantic_values.size());

      for (size_t semantic_value_index = 0; semantic_value_index < semantic_values.size(); ++semantic_value_index) {
        statement_variants.push_back(CastSemanticValueTo<pl_ast::StatementVariant>(semantic_values, semantic_value_index));
      }

      return pl_ast::StatementVariant{std::make_shared<pl_ast::Block>(pl_ast::Block{.statements_ = std::move(statement_variants)})};
    };

    // Using designated initializers for clarity & safety against changing field positions
    parser["Function"] = [](const peg::SemanticValues& semantic_values) {
      return pl_ast::Function{
        .identifier_ = CastSemanticValueTo<pl_ast::Identifier>(semantic_values, 0),
        .function_block_ = std::get<pl_ast::BlockPointer>(CastSemanticValueTo<pl_ast::StatementVariant>(semantic_values, 1)),
      };
    };

    parser["Program"] = [](const peg::SemanticValues& semantic_values) {
      pl_ast::Program program;
      const size_t number_of_functions = semantic_values.size();
      program.functions_.reserve(number_of_functions);
      for (size_t current_function = 0; current_function < number_of_functions; ++current_function) {
        program.functions_.push_back(CastSemanticValueTo<pl_ast::Function>(semantic_values, current_function));
      }
      return program;
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
} // namespace parser

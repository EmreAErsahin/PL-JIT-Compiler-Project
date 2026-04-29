#include "peglib.h"

#include <concepts>
#include <any>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "../common/overloaded.h"
#include "language_grammar.h"
#include "parser.h"

namespace parser {
  using InfixOperator = std::variant<ast::ArithmeticOperator, ast::RelationalOperator, ast::EqualityOperator, ast::LogicalOperator>;

  // Parsing library requires copyable types
  template <std::copy_constructible T>
  T CastSemanticValueTo(const std::any& value) {
    return std::any_cast<T>(value);
  }

  template <typename T>
  T ExtractStatementNode(const std::any& value) {
    return std::get<T>(CastSemanticValueTo<ast::StatementVariant>(value));
  }

  template <typename ExpressionNode, typename Operator>
  ast::ExpressionVariant
  MakeBinaryExpression(ast::ExpressionVariant left_operand, const Operator operator_value, ast::ExpressionVariant right_operand) {
    return ast::ExpressionVariant{
      ExpressionNode{
                     .left_operand_ = std::make_shared<ast::ExpressionVariant>(std::move(left_operand)),
                     .operator_ = operator_value,
                     .right_operand_ = std::make_shared<ast::ExpressionVariant>(std::move(right_operand)),
                     }
    };
  }

  ast::PrintStatement MakePrintStatement(const peg::SemanticValues& semantic_values, const bool new_line) {
    ast::PrintStatement print_statement;
    print_statement.new_line_ = new_line;
    if (!semantic_values.empty()) {
      print_statement.print_expression_ = CastSemanticValueTo<ast::ExpressionVariant>(semantic_values[0]);
    }
    return print_statement;
  }

  peg::parser MakeParser() {
    peg::parser parser;

    if (!parser.load_grammar(parser_grammar::kPegGrammar)) {
      throw std::runtime_error("MakeParser: failed to load parser grammar");
    }

    parser["IdentifierToken"] = [](const peg::SemanticValues& semantic_values) {
      return ast::Identifier{semantic_values.token_to_string()};
    };

    parser["Function"] = [](const peg::SemanticValues& semantic_values) {
      ast::Function function;

      function.identifier_ = CastSemanticValueTo<ast::Identifier>(semantic_values[0]);
      function.parameters_ = CastSemanticValueTo<std::vector<ast::Identifier>>(semantic_values[1]);

      function.function_block_ = ExtractStatementNode<ast::BlockPointer>(semantic_values[2]);

      return function;
    };

    parser["ParameterList"] = [](const peg::SemanticValues& semantic_values) {
      std::vector<ast::Identifier> parameters;
      parameters.reserve(semantic_values.size());

      for (const auto& semantic_value : semantic_values) {
        parameters.push_back(CastSemanticValueTo<ast::Identifier>(semantic_value));
      }

      return parameters;
    };

    parser["EmptyParameters"] = [](const peg::SemanticValues&) { return std::vector<ast::Identifier>{}; };

    parser["PrintStatement"] = [](const peg::SemanticValues& semantic_values) {
      return ast::StatementVariant{MakePrintStatement(semantic_values, false)};
    };

    parser["PrintlnStatement"] = [](const peg::SemanticValues& semantic_values) {
      return ast::StatementVariant{MakePrintStatement(semantic_values, true)};
    };

    parser["LetStatement"] = [](const peg::SemanticValues& semantic_values) {
      return ast::StatementVariant{
        ast::LetStatement{
                          .identifier_ = CastSemanticValueTo<ast::Identifier>(semantic_values[0]),
                          .initializer_expression_ = CastSemanticValueTo<ast::ExpressionVariant>(semantic_values[1]),
                          }
      };
    };

    parser["AssignmentStatement"] = [](const peg::SemanticValues& semantic_values) {
      return ast::StatementVariant{
        ast::AssignmentStatement{
                                 .identifier_ = CastSemanticValueTo<ast::Identifier>(semantic_values[0]),
                                 .assigned_expression_ = CastSemanticValueTo<ast::ExpressionVariant>(semantic_values[1]),
                                 }
      };
    };

    parser["IfStatement"] = [](const peg::SemanticValues& semantic_values) {
      // First two semantic values are if, last one is else if it's present, everything in the middle is else ifs
      auto if_condition = CastSemanticValueTo<ast::ExpressionVariant>(semantic_values[0]);
      auto if_block = ExtractStatementNode<ast::BlockPointer>(semantic_values[1]);

      ast::ElseIfConditionBlockPairs else_if_branches;
      // Semantic values are: if condition, if block, zero or more else-if condition/block pairs,
      // and an optional trailing else block.
      const bool has_else_block = semantic_values.size() % 2 == 1;
      const size_t else_if_end = has_else_block ? semantic_values.size() - 1 : semantic_values.size();
      for (size_t branch_index = 2; branch_index < else_if_end; branch_index += 2) {
        else_if_branches.emplace_back(
          CastSemanticValueTo<ast::ExpressionVariant>(semantic_values[branch_index]),
          ExtractStatementNode<ast::BlockPointer>(semantic_values[branch_index + 1])
        );
      }

      std::optional<ast::BlockPointer> else_block;
      if (has_else_block) {
        else_block = ExtractStatementNode<ast::BlockPointer>(semantic_values.back());
      }
      return ast::StatementVariant{
        ast::IfStatement{
                         .if_condition_ = std::move(if_condition),
                         .if_block_ = std::move(if_block),
                         .else_if_branches_ = std::move(else_if_branches),
                         .else_block_ = std::move(else_block),
                         }
      };
    };

    parser["WhileStatement"] = [](const peg::SemanticValues& semantic_values) {
      auto while_condition = CastSemanticValueTo<ast::ExpressionVariant>(semantic_values[0]);
      auto while_block = ExtractStatementNode<ast::BlockPointer>(semantic_values[1]);

      return ast::StatementVariant{
        ast::WhileStatement{.while_condition_ = std::move(while_condition), .while_block_ = std::move(while_block)}
      };
    };

    parser["ForStatement"] = [](const peg::SemanticValues& semantic_values) {
      // Statements are all held as statement variant, so we cant any cast straight to let statement / assignment statement
      auto initializer = ExtractStatementNode<ast::LetStatement>(semantic_values[0]);
      auto condition = CastSemanticValueTo<ast::ExpressionVariant>(semantic_values[1]);
      auto update = ExtractStatementNode<ast::AssignmentStatement>(semantic_values[2]);
      auto for_block = ExtractStatementNode<ast::BlockPointer>(semantic_values[3]);

      return ast::StatementVariant{
        ast::ForStatement{
                          .initializer_ = std::move(initializer),
                          .condition_ = std::move(condition),
                          .update_ = std::move(update),
                          .for_block_ = std::move(for_block)
        }
      };
    };

    parser["ForUpdate"] = [](const peg::SemanticValues& semantic_values) {
      return ast::StatementVariant{
        ast::AssignmentStatement{
                                 .identifier_ = CastSemanticValueTo<ast::Identifier>(semantic_values[0]),
                                 .assigned_expression_ = CastSemanticValueTo<ast::ExpressionVariant>(semantic_values[1]),
                                 }
      };
    };

    parser["ContinueStatement"] = [](const peg::SemanticValues&) { return ast::StatementVariant{ast::ContinueStatement{}}; };

    parser["BreakStatement"] = [](const peg::SemanticValues&) { return ast::StatementVariant{ast::BreakStatement{}}; };

    parser["ReturnStatement"] = [](const peg::SemanticValues& semantic_values) {
      ast::ReturnStatement return_statement;
      if (!semantic_values.empty()) {
        return_statement.return_expression_ = CastSemanticValueTo<ast::ExpressionVariant>(semantic_values[0]);
      }
      return ast::StatementVariant{return_statement};
    };

    parser["FunctionCallStatement"] = [](const peg::SemanticValues& semantic_values) {
      return ast::StatementVariant{ast::FunctionCallStatement{
        .function_call_ = std::get<ast::FunctionCallExpression>(CastSemanticValueTo<ast::ExpressionVariant>(semantic_values[0]))
      }};
    };

    parser["Integer"] = [](const peg::SemanticValues& semantic_values) {
      return ast::ExpressionVariant{ast::IntegerLiteralExpression{semantic_values.token_to_number<int64_t>()}};
    };

    parser["Bool"] = [](const peg::SemanticValues& semantic_values) {
      return ast::ExpressionVariant{ast::BoolLiteralExpression{semantic_values.choice() == 0}};
    };

    parser["Nothing"] = [](const peg::SemanticValues&) { return ast::ExpressionVariant{ast::NothingLiteralExpression{}}; };

    parser["FunctionCallExpression"] = [](const peg::SemanticValues& semantic_values) {
      ast::FunctionCallExpression function_call_expression;
      function_call_expression.function_name_ = CastSemanticValueTo<ast::Identifier>(semantic_values[0]);
      function_call_expression.arguments_ = CastSemanticValueTo<std::vector<ast::CopyableExpressionPointer>>(semantic_values[1]);

      return ast::ExpressionVariant{std::move(function_call_expression)};
    };

    parser["ArgumentList"] = [](const peg::SemanticValues& semantic_values) {
      std::vector<ast::CopyableExpressionPointer> arguments;
      arguments.reserve(semantic_values.size());

      for (const auto& semantic_value : semantic_values) {
        arguments.push_back(std::make_shared<ast::ExpressionVariant>(CastSemanticValueTo<ast::ExpressionVariant>(semantic_value)));
      }

      return arguments;
    };

    parser["EmptyArguments"] = [](const peg::SemanticValues&) { return std::vector<ast::CopyableExpressionPointer>{}; };

    parser["IdentifierExpression"] = [](const peg::SemanticValues& semantic_values) {
      return ast::ExpressionVariant{ast::IdentifierExpression{CastSemanticValueTo<ast::Identifier>(semantic_values[0])}};
    };

    parser["UnaryOperator"] = [](const peg::SemanticValues& semantic_values) {
      const auto operator_token = semantic_values.token_to_string();
      if (operator_token == "-") {
        return ast::UnaryOperator::kNegate;
      } else if (operator_token == "!") {
        return ast::UnaryOperator::kNot;
      }
      throw std::runtime_error("UnaryOperator: unsupported operator");
    };

    parser["UnaryExpression"] = [](const peg::SemanticValues& semantic_values) {
      // We can never have a unary operator on its own, so we know this is a sole expression (atom in grammar)
      if (semantic_values.size() == 1) {
        return CastSemanticValueTo<ast::ExpressionVariant>(semantic_values[0]);
      }

      return ast::ExpressionVariant{
        ast::UnaryExpression{
                             .operator_ = CastSemanticValueTo<ast::UnaryOperator>(semantic_values[0]),
                             .operand_ = std::make_shared<ast::ExpressionVariant>(CastSemanticValueTo<ast::ExpressionVariant>(semantic_values[1])),
                             }
      };
    };

    parser["InfixOperator"] = [](const peg::SemanticValues& semantic_values) {
      const auto operator_token = semantic_values.token_to_string();
      if (operator_token == "+") {
        return InfixOperator{ast::ArithmeticOperator::kAdd};
      } else if (operator_token == "-") {
        return InfixOperator{ast::ArithmeticOperator::kSubtract};
      } else if (operator_token == "*") {
        return InfixOperator{ast::ArithmeticOperator::kMultiply};
      } else if (operator_token == "/") {
        return InfixOperator{ast::ArithmeticOperator::kDivide};
      } else if (operator_token == "%") {
        return InfixOperator{ast::ArithmeticOperator::kModulo};
      } else if (operator_token == "<") {
        return InfixOperator{ast::RelationalOperator::kLessThan};
      } else if (operator_token == "<=") {
        return InfixOperator{ast::RelationalOperator::kLessThanOrEqual};
      } else if (operator_token == ">") {
        return InfixOperator{ast::RelationalOperator::kGreaterThan};
      } else if (operator_token == ">=") {
        return InfixOperator{ast::RelationalOperator::kGreaterThanOrEqual};
      } else if (operator_token == "==") {
        return InfixOperator{ast::EqualityOperator::kEqual};
      } else if (operator_token == "!=") {
        return InfixOperator{ast::EqualityOperator::kNotEqual};
      } else if (operator_token == "&&") {
        return InfixOperator{ast::LogicalOperator::kAnd};
      } else if (operator_token == "||") {
        return InfixOperator{ast::LogicalOperator::kOr};
      }
      throw std::runtime_error("InfixOperator: unsupported operator");
    };

    parser["Atom"] = [](const peg::SemanticValues& semantic_values) {
      return CastSemanticValueTo<ast::ExpressionVariant>(semantic_values[0]);
    };

    parser["Expression"] = [](const peg::SemanticValues& semantic_values) {
      return CastSemanticValueTo<ast::ExpressionVariant>(semantic_values[0]);
    };

    parser["InfixExpression"] = [](const peg::SemanticValues& semantic_values) {
      auto left_operand = CastSemanticValueTo<ast::ExpressionVariant>(semantic_values[0]);

      if (semantic_values.size() == 1) {
        return left_operand;
      }

      const auto infix_operator = CastSemanticValueTo<InfixOperator>(semantic_values[1]);
      auto right_operand = CastSemanticValueTo<ast::ExpressionVariant>(semantic_values[2]);

      return std::visit(
        template_helpers::Overloaded{
          [&left_operand, &right_operand](const ast::ArithmeticOperator operator_value) -> ast::ExpressionVariant {
            return MakeBinaryExpression<ast::ArithmeticExpression>(std::move(left_operand), operator_value, std::move(right_operand));
          },
          [&left_operand, &right_operand](const ast::RelationalOperator operator_value) -> ast::ExpressionVariant {
            return MakeBinaryExpression<ast::RelationalExpression>(std::move(left_operand), operator_value, std::move(right_operand));
          },
          [&left_operand, &right_operand](const ast::EqualityOperator operator_value) -> ast::ExpressionVariant {
            return MakeBinaryExpression<ast::EqualityExpression>(std::move(left_operand), operator_value, std::move(right_operand));
          },
          [&left_operand, &right_operand](const ast::LogicalOperator operator_value) -> ast::ExpressionVariant {
            return MakeBinaryExpression<ast::LogicalExpression>(std::move(left_operand), operator_value, std::move(right_operand));
          },
        },
        infix_operator
      );
    };

    parser["Statement"] = [](const peg::SemanticValues& semantic_values) {
      return CastSemanticValueTo<ast::StatementVariant>(semantic_values[0]);
    };

    parser["Block"] = [](const peg::SemanticValues& semantic_values) {
      std::vector<ast::StatementVariant> statement_variants;
      statement_variants.reserve(semantic_values.size());

      for (const auto& semantic_value : semantic_values) {
        statement_variants.push_back(CastSemanticValueTo<ast::StatementVariant>(semantic_value));
      }

      return ast::StatementVariant{std::make_shared<ast::Block>(ast::Block{.statements_ = std::move(statement_variants)})};
    };

    parser["Program"] = [](const peg::SemanticValues& semantic_values) {
      ast::Program program;
      program.functions_.reserve(semantic_values.size());
      for (const auto& semantic_value : semantic_values) {
        program.functions_.push_back(CastSemanticValueTo<ast::Function>(semantic_value));
      }
      return program;
    };

    // Trades memory usage for better speed with memoization of grammar rules
    parser.enable_packrat_parsing();

    return parser;
  }

  ast::Program ParseFileContentsIntoAST(std::string_view file_contents) {
    auto parser = MakeParser();

    size_t error_line = 0;
    size_t error_column = 0;
    std::string error_message;

    parser.set_logger([&error_line, &error_column, &error_message](const size_t line, const size_t column, const std::string& message) {
      error_line = line;
      error_column = column;
      error_message = message;
    });

    ast::Program program_ast;
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

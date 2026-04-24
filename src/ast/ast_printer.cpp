#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>

#include "../common/overloaded.h"
#include "ast.h"
#include "ast_printer.h"

namespace ast_walk {
  std::string Indentation(const size_t depth) {
    return std::string(depth, '\t');
  }

  std::string ToString(const pl_ast::ExpressionVariant& expression_variant);

  std::string ToString(const std::vector<pl_ast::Identifier>& identifiers) {
    std::string joined_identifiers;
    for (size_t current_identifier = 0; current_identifier < identifiers.size(); ++current_identifier) {
      if (current_identifier != 0) {
        joined_identifiers += ", ";
      }
      joined_identifiers += identifiers[current_identifier].name_;
    }
    return joined_identifiers;
  }

  std::string ToString(const std::vector<pl_ast::CopyableExpressionPointer>& arguments) {
    std::string joined_arguments;
    for (size_t current_argument = 0; current_argument < arguments.size(); ++current_argument) {
      if (current_argument != 0) {
        joined_arguments += ", ";
      }
      joined_arguments += ToString(*arguments[current_argument]);
    }
    return joined_arguments;
  }

  constexpr std::string_view ToString(const pl_ast::ArithmeticOperator arithmetic_operator) {
    switch (arithmetic_operator) {
      case pl_ast::ArithmeticOperator::kAdd: return "+";
      case pl_ast::ArithmeticOperator::kSubtract: return "-";
      case pl_ast::ArithmeticOperator::kMultiply: return "*";
      case pl_ast::ArithmeticOperator::kDivide: return "/";
    }

    throw std::runtime_error("ToString: unsupported arithmetic operator");
  }

  constexpr std::string_view ToString(const pl_ast::RelationalOperator relational_operator) {
    switch (relational_operator) {
      case pl_ast::RelationalOperator::kLessThan: return "<";
      case pl_ast::RelationalOperator::kLessThanOrEqual: return "<=";
      case pl_ast::RelationalOperator::kGreaterThan: return ">";
      case pl_ast::RelationalOperator::kGreaterThanOrEqual: return ">=";
    }

    throw std::runtime_error("ToString: unsupported relational operator");
  }

  constexpr std::string_view ToString(const pl_ast::EqualityOperator equality_operator) {
    switch (equality_operator) {
      case pl_ast::EqualityOperator::kEqual: return "==";
      case pl_ast::EqualityOperator::kNotEqual: return "!=";
    }

    throw std::runtime_error("ToString: unsupported equality operator");
  }

  constexpr std::string_view ToString(const pl_ast::LogicalOperator logical_operator) {
    switch (logical_operator) {
      case pl_ast::LogicalOperator::kAnd: return "&&";
      case pl_ast::LogicalOperator::kOr: return "||";
    }

    throw std::runtime_error("ToString: unsupported logical operator");
  }

  std::string ToString(const pl_ast::ExpressionVariant& expression_variant) {
    return std::visit(
      template_helpers::Overloaded{
        [](const pl_ast::IntegerLiteralExpression& integer_expression) -> std::string { return std::to_string(integer_expression.value_); },
        [](const pl_ast::BoolLiteralExpression& bool_expression) -> std::string { return bool_expression.value_ ? "true" : "false"; },
        [](const pl_ast::NothingLiteralExpression&) -> std::string { return "nothing"; },
        [](const pl_ast::IdentifierExpression& identifier_expression) -> std::string { return identifier_expression.identifier_.name_; },
        [](const pl_ast::ArithmeticExpression& arithmetic_expression) -> std::string {
          return std::format(
            "({} {} {})", ToString(*arithmetic_expression.left_operand_), ToString(arithmetic_expression.operator_),
            ToString(*arithmetic_expression.right_operand_)
          );
        },
        [](const pl_ast::RelationalExpression& relational_expression) -> std::string {
          return std::format(
            "({} {} {})", ToString(*relational_expression.left_operand_), ToString(relational_expression.operator_),
            ToString(*relational_expression.right_operand_)
          );
        },
        [](const pl_ast::EqualityExpression& equality_expression) -> std::string {
          return std::format(
            "({} {} {})", ToString(*equality_expression.left_operand_), ToString(equality_expression.operator_),
            ToString(*equality_expression.right_operand_)
          );
        },
        [](const pl_ast::LogicalExpression& logical_expression) -> std::string {
          return std::format(
            "({} {} {})", ToString(*logical_expression.left_operand_), ToString(logical_expression.operator_),
            ToString(*logical_expression.right_operand_)
          );
        },
        [](const pl_ast::FunctionCallExpression& function_call_expression) -> std::string {
          return std::format("{}({})", function_call_expression.function_name_.name_, ToString(function_call_expression.arguments_));
        },
        [](const auto&) -> std::string { throw std::runtime_error("ToString: unsupported expression type"); }
      },
      expression_variant
    );
  }

  std::string ToString(const pl_ast::BlockPointer& block_pointer, const size_t depth) {
    if (!block_pointer) {
      throw std::runtime_error("ToString: null block pointer");
    }

    std::string block_string;
    for (const auto& statement_variant : block_pointer->statements_) {
      std::visit(
        template_helpers::Overloaded{
          [&block_string, depth](const pl_ast::PrintStatement& print_statement) {
            block_string += Indentation(depth) + (print_statement.new_line_ ? "println(" : "print(");
            if (print_statement.print_expression_) {
              block_string += ToString(*print_statement.print_expression_);
            }
            block_string += ");\n";
          },
          [&block_string, depth](const pl_ast::LetStatement& let_statement) {
            block_string += std::format(
              "{}let {} = {};\n", Indentation(depth), let_statement.identifier_.name_, ToString(let_statement.initializer_expression_)
            );
          },
          [&block_string, depth](const pl_ast::AssignmentStatement& assignment_statement) {
            block_string += std::format(
              "{}{} = {};\n", Indentation(depth), assignment_statement.identifier_.name_,
              ToString(assignment_statement.assigned_expression_)
            );
          },
          [&block_string, depth](const pl_ast::IfStatement& if_statement) {
            block_string += std::format("{}if {} {{\n", Indentation(depth), ToString(if_statement.if_condition_));
            block_string += ToString(if_statement.if_block_, depth + 1);
            block_string += Indentation(depth) + "}";

            for (const auto& [else_if_condition, else_if_block] : if_statement.else_if_branches_) {
              block_string += std::format(" else if {} {{\n", ToString(else_if_condition));
              block_string += ToString(else_if_block, depth + 1);
              block_string += Indentation(depth) + "}";
            }

            if (if_statement.else_block_) {
              block_string += " else {\n";
              block_string += ToString(*if_statement.else_block_, depth + 1);
              block_string += Indentation(depth) + "}";
            }

            block_string += "\n";
          },
          [&block_string, depth](const pl_ast::WhileStatement& while_statement) {
            block_string += std::format("{}while {} {{\n", Indentation(depth), ToString(while_statement.while_condition_));
            block_string += ToString(while_statement.while_block_, depth + 1);
            block_string += Indentation(depth) + "}\n";
          },
          [&block_string, depth](const pl_ast::ForStatement& for_statement) {
            block_string += std::format(
              "{}for let {} = {}; {}; {} = {} {{\n", Indentation(depth), for_statement.initializer_.identifier_.name_,
              ToString(for_statement.initializer_.initializer_expression_), ToString(for_statement.condition_),
              for_statement.update_.identifier_.name_, ToString(for_statement.update_.assigned_expression_)
            );
            block_string += ToString(for_statement.for_block_, depth + 1);
            block_string += Indentation(depth) + "}\n";
          },
          [&block_string, depth](const pl_ast::ContinueStatement&) { block_string += Indentation(depth) + "continue;\n"; },
          [&block_string, depth](const pl_ast::BreakStatement&) { block_string += Indentation(depth) + "break;\n"; },
          [&block_string, depth](const pl_ast::ReturnStatement& return_statement) {
            block_string += Indentation(depth) + "return";
            if (return_statement.return_expression_) {
              block_string += " " + ToString(*return_statement.return_expression_);
            }
            block_string += ";\n";
          },
          [&block_string, depth](const pl_ast::BlockPointer& nested_block_pointer) {
            block_string += Indentation(depth) + "{\n";
            block_string += ToString(nested_block_pointer, depth + 1);
            block_string += Indentation(depth) + "}\n";
          },
          [&block_string, depth](const pl_ast::FunctionCallStatement& function_call_statement) {
            block_string += std::format("{}{};\n", Indentation(depth), ToString(pl_ast::ExpressionVariant{function_call_statement.function_call_}));
          }
        },
        statement_variant
      );
    }
    return block_string;
  }

  std::string ToString(const pl_ast::Program& program) {
    std::string program_string;
    for (const auto& function : program.functions_) {
      program_string += std::format(
        "fn {}({}) {{\n{}}}\n", function.identifier_.name_, ToString(function.parameters_), ToString(function.function_block_, 1)
      );
    }
    return program_string;
  }
} // namespace ast_walk

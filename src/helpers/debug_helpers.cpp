#include <format>
#include <stdexcept>
#include <string>
#include <variant>

#include "../pl_ast.h"
#include "debug_helpers.h"
#include "overloaded.h"

namespace debug_helpers {
  std::string Indentation(const size_t depth) {
    return std::string(depth, '\t');
  }

  std::string ToString(const pl_ast::ArithmeticOperator arithmetic_operator) {
    switch (arithmetic_operator) {
      case pl_ast::ArithmeticOperator::kAdd: return "+";
      case pl_ast::ArithmeticOperator::kSubtract: return "-";
      case pl_ast::ArithmeticOperator::kMultiply: return "*";
      case pl_ast::ArithmeticOperator::kDivide: return "/";
    }

    throw std::runtime_error("ToString: unsupported arithmetic operator");
  }

  std::string ToString(const pl_ast::RelationalOperator relational_operator) {
    switch (relational_operator) {
      case pl_ast::RelationalOperator::kLessThan: return "<";
      case pl_ast::RelationalOperator::kLessThanOrEqual: return "<=";
      case pl_ast::RelationalOperator::kGreaterThan: return ">";
      case pl_ast::RelationalOperator::kGreaterThanOrEqual: return ">=";
    }

    throw std::runtime_error("ToString: unsupported relational operator");
  }

  std::string ToString(const pl_ast::EqualityOperator equality_operator) {
    switch (equality_operator) {
      case pl_ast::EqualityOperator::kEqual: return "==";
      case pl_ast::EqualityOperator::kNotEqual: return "!=";
    }

    throw std::runtime_error("ToString: unsupported equality operator");
  }

  std::string ToString(const pl_ast::LogicalOperator logical_operator) {
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
          return "(" + ToString(*arithmetic_expression.left_operand_) + " " + ToString(arithmetic_expression.operator_) + " "
                 + ToString(*arithmetic_expression.right_operand_) + ")";
        },
        [](const pl_ast::RelationalExpression& relational_expression) -> std::string {
          return "(" + ToString(*relational_expression.left_operand_) + " " + ToString(relational_expression.operator_) + " "
                 + ToString(*relational_expression.right_operand_) + ")";
        },
        [](const pl_ast::EqualityExpression& equality_expression) -> std::string {
          return "(" + ToString(*equality_expression.left_operand_) + " " + ToString(equality_expression.operator_) + " "
                 + ToString(*equality_expression.right_operand_) + ")";
        },
        [](const pl_ast::LogicalExpression& logical_expression) -> std::string {
          return "(" + ToString(*logical_expression.left_operand_) + " " + ToString(logical_expression.operator_) + " "
                 + ToString(*logical_expression.right_operand_) + ")";
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
          [&block_string, depth](const pl_ast::DebugPrintStatement& debug_print_statement) {
            block_string += Indentation(depth) + "debugPrint(";
            if (debug_print_statement.expression_) {
              block_string += ToString(*debug_print_statement.expression_);
            }
            block_string += ");\n";
          },
          [&block_string, depth](const pl_ast::LetStatement& let_statement) {
            block_string += Indentation(depth) + "let " + let_statement.identifier_.name_ + " = "
                            + ToString(let_statement.initializer_expression_) + ";\n";
          },
          [&block_string, depth](const pl_ast::AssignmentStatement& assignment_statement) {
            block_string += Indentation(depth) + assignment_statement.identifier_.name_ + " = "
                            + ToString(assignment_statement.assigned_expression_) + ";\n";
          },
          [&block_string, depth](const pl_ast::BlockPointer& nested_block_pointer) {
            block_string += Indentation(depth) + "{\n";
            block_string += ToString(nested_block_pointer, depth + 1);
            block_string += Indentation(depth) + "}\n";
          }
        },
        statement_variant
      );
    }
    return block_string;
  }

  std::string ToString(const pl_ast::Program& program) {
    return std::format("fn {}() {{\n{}}}\n", program.function_.identifier_.name_, ToString(program.function_.function_block_, 1));
  }
} // namespace debug_helpers

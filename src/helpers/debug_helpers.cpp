#include <string>
#include <stdexcept>
#include <variant>

#include "../pl_ast.h"
#include "debug_helpers.h"
#include "overloaded.h"

namespace debug_helpers {
  std::string ToString(const pl_ast::ArithmeticOperator arithmetic_operator) {
    switch (arithmetic_operator) {
      case pl_ast::ArithmeticOperator::ADD: return "+";
      case pl_ast::ArithmeticOperator::SUBTRACT: return "-";
      case pl_ast::ArithmeticOperator::MULTIPLY: return "*";
      case pl_ast::ArithmeticOperator::DIVIDE: return "/";
    }

    throw std::runtime_error("ToString: unsupported arithmetic operator");
  }

  std::string ToString(const pl_ast::ExpressionVariant& expression_variant) {
    return std::visit(
      template_helpers::Overloaded{
        [](const pl_ast::IntegerLiteralExpression& integer_expression) -> std::string { return std::to_string(integer_expression.value_); },
        [](const pl_ast::BoolLiteralExpression& bool_expression) -> std::string { return bool_expression.value_ ? "true" : "false"; },
        [](const pl_ast::NothingLiteralExpression&) -> std::string { return "nothing"; },
        [](const pl_ast::IdentifierExpression& identifier_expression) -> std::string { return identifier_expression.identifier_.name_; },
        [](const pl_ast::BinaryExpression& binary_expression) -> std::string {
          return "(" + ToString(*binary_expression.left_operand_) + " " + ToString(binary_expression.operator_) + " "
                 + ToString(*binary_expression.right_operand_) + ")";
        },
        [](const auto&) -> std::string { throw std::runtime_error("ToString: unsupported expression type"); }
      },
      expression_variant
    );
  }

  // TODO: Far from complete. Needs dynamic return types, parameters, multi-function, etc... Quick solution
  std::string ToString(const pl_ast::Program& program) {
    std::string program_string = "fn " + program.function_.identifier_.name_ + "() {\n";

    for (const auto& statement_variant : program.function_.statements_) {
      std::visit(
        template_helpers::Overloaded{
          [&program_string](const pl_ast::DebugPrintStatement& debug_print_statement) {
            program_string += "\tdebugPrint(";
            if (debug_print_statement.expression_) {
              program_string += ToString(*debug_print_statement.expression_);
            }
            program_string += ");\n";
          },
          [&program_string](const pl_ast::LetStatement& let_statement) {
            program_string += "\tlet " + let_statement.identifier_.name_ + " = " + ToString(let_statement.initializer_expression_) + ";\n";
          },
          [&program_string](const pl_ast::AssignmentStatement& assignment_statement) {
            program_string += "\t" + assignment_statement.identifier_.name_ + " = "
                              + ToString(assignment_statement.assigned_expression_) + ";\n";
          },
        },
        statement_variant
      );
    }
    program_string += "}\n";
    return program_string;
  }
} // namespace debug_helpers

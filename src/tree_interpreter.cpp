#include <iostream>
#include <stdexcept>
#include <string>
#include <variant>

#include "helpers/template_helpers.h"
#include "pl_ast.h"
#include "tree_interpreter.h"

namespace tree_interpreter {
  struct NothingValue {};

  using Value = std::variant<int64_t, bool, NothingValue>;

  int64_t RequireInteger(const Value& value) {
    if (const auto integer_value = std::get_if<int64_t>(&value)) {
      return *integer_value;
    }

    throw std::runtime_error("RequireInteger: arithmetic requires integer operands");
  }

  Value ExecuteOperation(const int64_t left_value, const pl_ast::ArithmeticOperator arithmetic_operator, const int64_t right_value) {
    switch (arithmetic_operator) {
      case pl_ast::ArithmeticOperator::ADD: return left_value + right_value;
      case pl_ast::ArithmeticOperator::SUBTRACT: return left_value - right_value;
      case pl_ast::ArithmeticOperator::MULTIPLY: return left_value * right_value;
      case pl_ast::ArithmeticOperator::DIVIDE:
        if (right_value == 0) {
          throw std::runtime_error("ExecuteOperation: division by zero");
        }
        return left_value / right_value;
    }

    throw std::runtime_error("ExecuteOperation: unsupported arithmetic operator");
  }

  Value EvaluateExpression(const pl_ast::ExpressionVariant& expression_variant) {
    return std::visit(
      template_helpers::Overloaded{
        [](const pl_ast::IntegerLiteralExpression& integer_expression) -> Value { return integer_expression.value_; },
        [](const pl_ast::BoolLiteralExpression& bool_expression) -> Value { return bool_expression.value_; },
        [](const pl_ast::NothingLiteralExpression&) -> Value { return NothingValue{}; },
        [](const pl_ast::BinaryExpression& binary_expression) -> Value {
          const auto left_value = RequireInteger(EvaluateExpression(*binary_expression.left_operand_));
          const auto right_value = RequireInteger(EvaluateExpression(*binary_expression.right_operand_));
          return ExecuteOperation(left_value, binary_expression.operator_, right_value);
        },
      },
      expression_variant
    );
  }

  void DebugPrintValue(const Value& value) {
    std::visit(
      template_helpers::Overloaded{
        [](const int64_t integer_value) { std::cout << integer_value; },
        [](const bool bool_value) { std::cout << (bool_value ? "true" : "false"); },
        [](const NothingValue&) { std::cout << "nothing"; },
      },
      value
    );
  }

  void ExecuteStatement(const pl_ast::StatementVariant& statement_variant) {
    std::visit(
      template_helpers::Overloaded{
        [](const pl_ast::DebugPrintStatement& debug_print_statement) {
          if (debug_print_statement.expression_) {
            DebugPrintValue(EvaluateExpression(*debug_print_statement.expression_));
          }
        },
      },
      statement_variant
    );
  }

  void ExecuteAstWithTreeInterpreter(const pl_ast::Program& program) {
    // Enforcing one main function
    const pl_ast::Function* main_function = nullptr;
    // TODO: Make this for once we have multiple functions
    if (program.function_.identifier_.name_ == "main") {
      main_function = &program.function_;
    }
    if (!main_function) {
      throw std::runtime_error("ExecuteAstWithTreeInterpreter: no main function found");
    }

    for (const auto& statement_variant : main_function->statements_) {
      ExecuteStatement(statement_variant);
    }
  }
} // namespace tree_interpreter

#include <iostream>
#include <stdexcept>
#include <string>
#include <variant>

#include "helpers/template_helpers.h"
#include "tree_interpreter.h"

namespace tree_interpreter {
  struct NothingValue {};

  using Value = std::variant<int64_t, bool, NothingValue>;

  Value EvaluateExpression(const pl_ast::ExpressionVariant& expression_variant) {
    return std::visit(
      template_helpers::Overloaded{
        [](const pl_ast::IntegerLiteralExpression& integer_expression) -> Value { return integer_expression.value_; },
        [](const pl_ast::BoolLiteralExpression& bool_expression) -> Value { return bool_expression.value_; },
        [](const pl_ast::NothingLiteralExpression&) -> Value { return NothingValue{}; },
      },
      expression_variant
    );
  }

  void PrintValue(const Value& value) {
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
        [](const pl_ast::PrintStatement& print_statement) {
          if (print_statement.expression_) {
            PrintValue(EvaluateExpression(*print_statement.expression_));
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

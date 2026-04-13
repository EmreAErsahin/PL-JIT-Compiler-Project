#include <iostream>
#include <ranges>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "helpers/overloaded.h"
#include "pl_ast.h"
#include "tree_interpreter.h"

namespace tree_interpreter {
  // Runtime types
  struct NothingValue {};

  using Value = std::variant<int64_t, bool, NothingValue>;
  using Scope = std::unordered_map<std::string, Value>;

  struct RuntimeState {
    std::vector<Scope> scopes_;
  };

  // Runtime helpers
  int64_t RequireAndGetInteger(const Value& value) {
    if (const auto integer_value = std::get_if<int64_t>(&value)) {
      return *integer_value;
    }

    throw std::runtime_error("RequireAndGetInteger: arithmetic requires integer operands");
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

  Value LookupVariable(const RuntimeState& runtime_state, const std::string& variable_name) {
    for (const auto& scope : std::views::reverse(runtime_state.scopes_)) {
      if (const auto variable_iterator = scope.find(variable_name); variable_iterator != scope.end()) {
        return variable_iterator->second;
      }
    }

    throw std::runtime_error("EvaluateExpression: unknown variable '" + variable_name + "'");
  }

  void DeclareVariable(RuntimeState& runtime_state, const std::string& variable_name, const Value& value) {
    auto& current_scope = runtime_state.scopes_.back();
    if (const auto variable_iterator = current_scope.find(variable_name); variable_iterator != current_scope.end()) {
      throw std::runtime_error("ExecuteStatement: variable '" + variable_name + "' already declared");
    }

    current_scope.emplace(variable_name, value);
  }

  void AssignVariable(RuntimeState& runtime_state, const std::string& variable_name, const Value& value) {
    for (auto& scope : std::views::reverse(runtime_state.scopes_)) {
      if (auto variable_iterator = scope.find(variable_name); variable_iterator != scope.end()) {
        variable_iterator->second = value;
        return;
      }
    }
    throw std::runtime_error("ExecuteStatement: variable '" + variable_name + "' is not declared");
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

  // Core execution
  Value EvaluateExpression(const RuntimeState& runtime_state, const pl_ast::ExpressionVariant& expression_variant) {
    return std::visit(
      template_helpers::Overloaded{
        [](const pl_ast::IntegerLiteralExpression& integer_expression) -> Value { return integer_expression.value_; },
        [](const pl_ast::BoolLiteralExpression& bool_expression) -> Value { return bool_expression.value_; },
        [](const pl_ast::NothingLiteralExpression&) -> Value { return NothingValue{}; },
        [&runtime_state](const pl_ast::IdentifierExpression& identifier_expression) -> Value {
          const auto& variable_name = identifier_expression.identifier_.name_;
          return LookupVariable(runtime_state, variable_name);
        },
        [&runtime_state](const pl_ast::BinaryExpression& binary_expression) -> Value {
          const auto left_value = RequireAndGetInteger(EvaluateExpression(runtime_state, *binary_expression.left_operand_));
          const auto right_value = RequireAndGetInteger(EvaluateExpression(runtime_state, *binary_expression.right_operand_));
          return ExecuteOperation(left_value, binary_expression.operator_, right_value);
        },
      },
      expression_variant
    );
  }

  void ExecuteBlock(RuntimeState& runtime_state, const pl_ast::BlockPointer& block_pointer);

  void ExecuteStatement(RuntimeState& runtime_state, const pl_ast::StatementVariant& statement_variant) {
    std::visit(
      template_helpers::Overloaded{
        [&runtime_state](const pl_ast::DebugPrintStatement& debug_print_statement) {
          if (debug_print_statement.expression_) {
            DebugPrintValue(EvaluateExpression(runtime_state, *debug_print_statement.expression_));
          }
        },
        [&runtime_state](const pl_ast::LetStatement& let_statement) {
          const auto& variable_name = let_statement.identifier_.name_;
          DeclareVariable(runtime_state, variable_name, EvaluateExpression(runtime_state, let_statement.initializer_expression_));
        },
        [&runtime_state](const pl_ast::AssignmentStatement& assignment_statement) {
          const auto& variable_name = assignment_statement.identifier_.name_;
          AssignVariable(runtime_state, variable_name, EvaluateExpression(runtime_state, assignment_statement.assigned_expression_));
        },
        [&runtime_state](const pl_ast::BlockPointer& block_pointer) { ExecuteBlock(runtime_state, block_pointer); },
      },
      statement_variant
    );
  }

  void ExecuteBlock(RuntimeState& runtime_state, const pl_ast::BlockPointer& block_pointer) {
    if (!block_pointer) {
      throw std::runtime_error("ExecuteBlock: null block pointer");
    }

    // Entering new scope
    runtime_state.scopes_.emplace_back();

    for (const auto& statement_variant : block_pointer->statements_) {
      ExecuteStatement(runtime_state, statement_variant);
    }

    // Leaving scope
    runtime_state.scopes_.pop_back();
  }

  void ExecuteAstWithTreeInterpreter(const pl_ast::Program& program) {
    // Enforcing one main function
    const pl_ast::Function* main_function = nullptr;
    // TODO: Make this for loop once we have multiple functions
    if (program.function_.identifier_.name_ == "main") {
      main_function = &program.function_;
    }
    if (!main_function) {
      throw std::runtime_error("ExecuteAstWithTreeInterpreter: no main function found");
    }

    RuntimeState runtime_state;
    ExecuteBlock(runtime_state, main_function->function_block_);
  }
} // namespace tree_interpreter

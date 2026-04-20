#include <iostream>
#include <ranges>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "helpers/overloaded.h"
#include "pl_ast.h"
#include "tree_interpreter.h"

namespace tree_interpreter {
  //
  // Runtime types
  //
  struct NothingValue {};

  using Value = std::variant<int64_t, bool, NothingValue>;
  using Scope = std::unordered_map<std::string, Value>;

  struct RuntimeState {
    std::vector<Scope> scopes_;
  };

  // Using RAII to add/delete scopes on creation of object / exit of scope
  struct ScopeGuard {
    explicit ScopeGuard(RuntimeState& runtime_state) : runtime_state_(runtime_state) { runtime_state_.scopes_.emplace_back(); }

    ~ScopeGuard() { runtime_state_.scopes_.pop_back(); }

    // Need to delete copy/move constructors to ensure we add/delete scopes properly
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard(ScopeGuard&&) = delete;
    ScopeGuard& operator=(ScopeGuard&&) = delete;
    
   private:
    RuntimeState& runtime_state_;
  };

  Value EvaluateExpression(const RuntimeState& runtime_state, const pl_ast::ExpressionVariant& expression_variant);

  //
  // Runtime helpers
  //
  std::pair<Value, Value> EvaluateBothOperands(
    const RuntimeState& runtime_state, const pl_ast::CopyableExpressionPointer& left_operand, const pl_ast::CopyableExpressionPointer& right_operand
  ) {
    return {EvaluateExpression(runtime_state, *left_operand), EvaluateExpression(runtime_state, *right_operand)};
  }

  std::pair<int64_t, int64_t> EvaluateBothOperandsAsIntegers(
    const RuntimeState& runtime_state, const pl_ast::CopyableExpressionPointer& left_operand, const pl_ast::CopyableExpressionPointer& right_operand
  ) {
    const auto [left_value, right_value] = EvaluateBothOperands(runtime_state, left_operand, right_operand);

    const auto left_integer = std::get_if<int64_t>(&left_value);
    const auto right_integer = std::get_if<int64_t>(&right_value);
    if (!left_integer || !right_integer) {
      throw std::runtime_error("EvaluateBothOperandsAsIntegers: operation requires integer operands");
    }

    return {*left_integer, *right_integer};
  }

  bool IsTruthy(const Value& value) {
    return std::visit(
      template_helpers::Overloaded{
        [](const int64_t integer_value) { return integer_value != 0; },
        [](const bool bool_value) { return bool_value; },
        [](const NothingValue&) { return false; },
      },
      value
    );
  }

  Value
  ExecuteArithmeticOperation(const int64_t left_value, const pl_ast::ArithmeticOperator arithmetic_operator, const int64_t right_value) {
    switch (arithmetic_operator) {
      case pl_ast::ArithmeticOperator::kAdd: return left_value + right_value;
      case pl_ast::ArithmeticOperator::kSubtract: return left_value - right_value;
      case pl_ast::ArithmeticOperator::kMultiply: return left_value * right_value;
      case pl_ast::ArithmeticOperator::kDivide:
        if (right_value == 0) {
          throw std::runtime_error("ExecuteArithmeticOperation: division by zero");
        }
        return left_value / right_value;
    }

    throw std::runtime_error("ExecuteArithmeticOperation: unsupported arithmetic operator");
  }

  Value
  ExecuteRelationalOperation(const int64_t left_value, const pl_ast::RelationalOperator relational_operator, const int64_t right_value) {
    switch (relational_operator) {
      case pl_ast::RelationalOperator::kLessThan: return left_value < right_value;
      case pl_ast::RelationalOperator::kLessThanOrEqual: return left_value <= right_value;
      case pl_ast::RelationalOperator::kGreaterThan: return left_value > right_value;
      case pl_ast::RelationalOperator::kGreaterThanOrEqual: return left_value >= right_value;
    }

    throw std::runtime_error("ExecuteRelationalOperation: unsupported relational operator");
  }

  Value ExecuteEqualityOperation(const Value& left_value, const pl_ast::EqualityOperator equality_operator, const Value& right_value) {
    const bool are_equal = std::visit(
      template_helpers::Overloaded{
        [](const int64_t left_integer, const int64_t right_integer) { return left_integer == right_integer; },
        [](const bool left_bool, const bool right_bool) { return left_bool == right_bool; },
        [](const NothingValue&, const NothingValue&) { return true; },
        [](const auto&, const auto&) {
          return false;
        }, // any mismatched types are never equal in this PL (equality means same type, same value)
      },
      left_value, right_value
    );

    switch (equality_operator) {
      case pl_ast::EqualityOperator::kEqual: return are_equal;
      case pl_ast::EqualityOperator::kNotEqual: return !are_equal;
    }

    throw std::runtime_error("ExecuteEqualityOperation: unsupported equality operator");
  }

  Value ExecuteLogicalOperation(const bool left_value, const pl_ast::LogicalOperator logical_operator, const bool right_value) {
    switch (logical_operator) {
      case pl_ast::LogicalOperator::kAnd: return left_value && right_value;
      case pl_ast::LogicalOperator::kOr: return left_value || right_value;
    }

    throw std::runtime_error("ExecuteLogicalOperation: unsupported logical operator");
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

  //
  // Core execution
  //
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
        [&runtime_state](const pl_ast::ArithmeticExpression& arithmetic_expression) -> Value {
          const auto [left_value, right_value] =
            EvaluateBothOperandsAsIntegers(runtime_state, arithmetic_expression.left_operand_, arithmetic_expression.right_operand_);
          return ExecuteArithmeticOperation(left_value, arithmetic_expression.operator_, right_value);
        },
        [&runtime_state](const pl_ast::RelationalExpression& relational_expression) -> Value {
          const auto [left_value, right_value] =
            EvaluateBothOperandsAsIntegers(runtime_state, relational_expression.left_operand_, relational_expression.right_operand_);
          return ExecuteRelationalOperation(left_value, relational_expression.operator_, right_value);
        },
        [&runtime_state](const pl_ast::EqualityExpression& equality_expression) -> Value {
          const auto [left_value, right_value] =
            EvaluateBothOperands(runtime_state, equality_expression.left_operand_, equality_expression.right_operand_);
          return ExecuteEqualityOperation(left_value, equality_expression.operator_, right_value);
        },
        [&runtime_state](const pl_ast::LogicalExpression& logical_expression) -> Value {
          const auto left_value = EvaluateExpression(runtime_state, *logical_expression.left_operand_);

          // Short-circuit evaluation
          if (logical_expression.operator_ == pl_ast::LogicalOperator::kAnd && !IsTruthy(left_value)) {
            return false;
          } else if (logical_expression.operator_ == pl_ast::LogicalOperator::kOr && IsTruthy(left_value)) {
            return true;
          }

          return ExecuteLogicalOperation(
            IsTruthy(left_value), logical_expression.operator_,
            IsTruthy(EvaluateExpression(runtime_state, *logical_expression.right_operand_))
          );
        }
      },
      expression_variant
    );
  }
  enum class ExecutionState { kNormal, kContinue, kBreak };

  ExecutionState ExecuteBlock(RuntimeState& runtime_state, const pl_ast::BlockPointer& block_pointer);

  ExecutionState ExecuteStatement(RuntimeState& runtime_state, const pl_ast::StatementVariant& statement_variant) {
    return std::visit(
      template_helpers::Overloaded{
        [&runtime_state](const pl_ast::DebugPrintStatement& debug_print_statement) {
          if (debug_print_statement.expression_) {
            DebugPrintValue(EvaluateExpression(runtime_state, *debug_print_statement.expression_));
          }
          return ExecutionState::kNormal;
        },
        [&runtime_state](const pl_ast::LetStatement& let_statement) {
          const auto& variable_name = let_statement.identifier_.name_;
          DeclareVariable(runtime_state, variable_name, EvaluateExpression(runtime_state, let_statement.initializer_expression_));
          return ExecutionState::kNormal;
        },
        [&runtime_state](const pl_ast::AssignmentStatement& assignment_statement) {
          const auto& variable_name = assignment_statement.identifier_.name_;
          AssignVariable(runtime_state, variable_name, EvaluateExpression(runtime_state, assignment_statement.assigned_expression_));
          return ExecutionState::kNormal;
        },
        [&runtime_state](const pl_ast::IfStatement& if_statement) {
          if (IsTruthy(EvaluateExpression(runtime_state, if_statement.if_condition_))) {
            return ExecuteBlock(runtime_state, if_statement.if_block_);
          }

          for (const auto& [else_if_condition, else_if_block] : if_statement.else_if_branches_) {
            if (IsTruthy(EvaluateExpression(runtime_state, else_if_condition))) {
              return ExecuteBlock(runtime_state, else_if_block);
            }
          }

          if (if_statement.else_block_) {
            return ExecuteBlock(runtime_state, *if_statement.else_block_);
          }
          return ExecutionState::kNormal;
        },
        [&runtime_state](const pl_ast::WhileStatement& while_statement) {
          while (IsTruthy(EvaluateExpression(runtime_state, while_statement.while_condition_))) {
            const auto execution_state = ExecuteBlock(runtime_state, while_statement.while_block_);

            if (execution_state == ExecutionState::kBreak) {
              break;
            }
          }
          return ExecutionState::kNormal;
        },
        [&runtime_state](const pl_ast::ForStatement& for_statement) {
          // Must add scope for initializer variable (needs to live past loop iterations)
          ScopeGuard initializer_variable_scope(runtime_state);
          ExecuteStatement(runtime_state, for_statement.initializer_);

          while (IsTruthy(EvaluateExpression(runtime_state, for_statement.condition_))) {
            const auto execution_state = ExecuteBlock(runtime_state, for_statement.for_block_);

            if (execution_state == ExecutionState::kBreak) {
              break;
            }

            ExecuteStatement(runtime_state, for_statement.update_);
          }
          return ExecutionState::kNormal;
        },
        [](const pl_ast::ContinueStatement&) { return ExecutionState::kContinue; },
        [](const pl_ast::BreakStatement&) { return ExecutionState::kBreak; },
        [&runtime_state](const pl_ast::BlockPointer& block_pointer) { return ExecuteBlock(runtime_state, block_pointer); },
      },
      statement_variant
    );
  }

  ExecutionState ExecuteBlock(RuntimeState& runtime_state, const pl_ast::BlockPointer& block_pointer) {
    if (!block_pointer) {
      throw std::runtime_error("ExecuteBlock: null block pointer");
    }

    ScopeGuard block_scope(runtime_state);

    auto status = ExecutionState::kNormal;
    // Stop executing the block as soon as break/continue needs to bubble up
    for (const auto& statement_variant : block_pointer->statements_) {
      status = ExecuteStatement(runtime_state, statement_variant);
      if (status != ExecutionState::kNormal) {
        break;
      }
    }

    return status;
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
    const ExecutionState execution_state = ExecuteBlock(runtime_state, main_function->function_block_);
    if (execution_state == ExecutionState::kContinue) {
      throw std::runtime_error("ExecuteAstWithTreeInterpreter: continue statement not within a loop");
    }
    if (execution_state == ExecutionState::kBreak) {
      throw std::runtime_error("ExecuteAstWithTreeInterpreter: break statement not within a loop");
    }
  }
} // namespace tree_interpreter

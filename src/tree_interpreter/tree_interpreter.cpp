#include <iostream>
#include <ranges>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "../common/overloaded.h"
#include "tree_interpreter.h"

namespace tree_interpreter {
  struct NothingValue {};

  // Runtime values (what expression are evaluated into)
  using Value = std::variant<int64_t, bool, NothingValue>;

  // {Variable Name, Variable Value}
  using Scope = std::unordered_map<std::string, Value>;

  // Scope for one function
  struct CallFrame {
    std::vector<Scope> scopes_;
  };

  enum class ControlFlow { kNormal, kContinue, kBreak };

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
        [](const auto&, const auto&) { return false; },
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

  class RuntimeState {
   public:
    struct ScopeGuard {
      // Using RAII to add/delete scopes on construction / destruction.
      explicit ScopeGuard(RuntimeState& runtime_state) : runtime_state_(runtime_state) {
        runtime_state_.CurrentFrame().scopes_.emplace_back();
      }

      ~ScopeGuard() { runtime_state_.CurrentFrame().scopes_.pop_back(); }

      // Delete copy/move so scope lifetime always matches one guard instance.
      ScopeGuard(const ScopeGuard&) = delete;
      ScopeGuard& operator=(const ScopeGuard&) = delete;
      ScopeGuard(ScopeGuard&&) = delete;
      ScopeGuard& operator=(ScopeGuard&&) = delete;

     private:
      RuntimeState& runtime_state_;
    };

    struct CallFrameGuard {
      explicit CallFrameGuard(RuntimeState& runtime_state) : runtime_state_(runtime_state) { runtime_state_.call_stack_.emplace_back(); }

      ~CallFrameGuard() { runtime_state_.call_stack_.pop_back(); }

      CallFrameGuard(const CallFrameGuard&) = delete;
      CallFrameGuard& operator=(const CallFrameGuard&) = delete;
      CallFrameGuard(CallFrameGuard&&) = delete;
      CallFrameGuard& operator=(CallFrameGuard&&) = delete;

     private:
      RuntimeState& runtime_state_;
    };

    const pl_ast::Function& BuildFunctionTableAndRequireMain(const pl_ast::Program& program) {
      for (const auto& current_function : program.functions_) {
        if (available_functions_.contains(current_function.identifier_.name_)) {
          throw std::runtime_error("BuildFunctionTableAndRequireMain: duplicate function '" + current_function.identifier_.name_ + "'");
        }
        available_functions_.emplace(current_function.identifier_.name_, &current_function);
      }

      if (!available_functions_.contains("main")) {
        throw std::runtime_error("BuildFunctionTableAndRequireMain: no main function found");
      }

      return *available_functions_.at("main");
    }

    const pl_ast::Function& LookupFunctionOrThrow(const std::string& function_name) const {
      if (const auto function_iterator = available_functions_.find(function_name); function_iterator != available_functions_.end()) {
        return *function_iterator->second;
      }

      throw std::runtime_error("EvaluateExpression: unknown function '" + function_name + "'");
    }

    const Value& LookupVariable(const std::string& variable_name) const {
      for (const auto& scope : std::views::reverse(call_stack_.back().scopes_)) {
        if (const auto variable_iterator = scope.find(variable_name); variable_iterator != scope.end()) {
          return variable_iterator->second;
        }
      }

      throw std::runtime_error("EvaluateExpression: unknown variable '" + variable_name + "'");
    }

    void DeclareVariable(const std::string& variable_name, Value value) {
      auto& current_scope = CurrentScope();
      if (const auto variable_iterator = current_scope.find(variable_name); variable_iterator != current_scope.end()) {
        throw std::runtime_error("ExecuteStatement: variable '" + variable_name + "' already declared");
      }

      current_scope.emplace(variable_name, value);
    }

    void AssignVariable(const std::string& variable_name, Value value) {
      for (auto& scope : std::views::reverse(CurrentFrame().scopes_)) {
        if (auto variable_iterator = scope.find(variable_name); variable_iterator != scope.end()) {
          variable_iterator->second = value;
          return;
        }
      }

      throw std::runtime_error("ExecuteStatement: variable '" + variable_name + "' is not declared");
    }

   private:
    // State needed for runtime execution: our function scopes + what functions are callable
    std::vector<CallFrame> call_stack_;
    std::unordered_map<std::string, const pl_ast::Function*> available_functions_;

    CallFrame& CurrentFrame() { return call_stack_.back(); }

    Scope& CurrentScope() { return CurrentFrame().scopes_.back(); }
  };

  class Interpreter {
   public:
    void Run(const pl_ast::Program& program) {
      const pl_ast::Function& main_function = runtime_state_.BuildFunctionTableAndRequireMain(program);
      ExecuteFunction(main_function);
    }

   private:
    RuntimeState runtime_state_;

    std::pair<Value, Value>
    EvaluateBothOperands(const pl_ast::CopyableExpressionPointer& left_operand, const pl_ast::CopyableExpressionPointer& right_operand) {
      return {EvaluateExpression(*left_operand), EvaluateExpression(*right_operand)};
    }

    std::pair<int64_t, int64_t> EvaluateBothOperandsAsIntegers(
      const pl_ast::CopyableExpressionPointer& left_operand, const pl_ast::CopyableExpressionPointer& right_operand
    ) {
      const auto [left_value, right_value] = EvaluateBothOperands(left_operand, right_operand);

      const auto left_integer = std::get_if<int64_t>(&left_value);
      const auto right_integer = std::get_if<int64_t>(&right_value);
      if (!left_integer || !right_integer) {
        throw std::runtime_error("EvaluateBothOperandsAsIntegers: operation requires integer operands");
      }

      return {*left_integer, *right_integer};
    }

    Value ExecuteFunction(const pl_ast::Function& function) {
      RuntimeState::CallFrameGuard call_frame_guard(runtime_state_);

      const auto control_flow = ExecuteBlock(function.function_block_);

      if (control_flow == ControlFlow::kContinue) {
        throw std::runtime_error("ExecuteFunction: continue statement not within a loop");
      }
      if (control_flow == ControlFlow::kBreak) {
        throw std::runtime_error("ExecuteFunction: break statement not within a loop");
      }

      // For now, we are always returning nothing
      return NothingValue{};
    }

    ControlFlow ExecuteBlock(const pl_ast::BlockPointer& block_pointer) {
      if (!block_pointer) {
        throw std::runtime_error("ExecuteBlock: null block pointer");
      }

      RuntimeState::ScopeGuard block_scope(runtime_state_);

      auto control_flow = ControlFlow::kNormal;
      for (const auto& statement_variant : block_pointer->statements_) {
        control_flow = ExecuteStatement(statement_variant);
        if (control_flow != ControlFlow::kNormal) {
          break;
        }
      }

      return control_flow;
    }

    ControlFlow ExecuteStatement(const pl_ast::StatementVariant& statement_variant) {
      return std::visit(
        template_helpers::Overloaded{
          [this](const pl_ast::DebugPrintStatement& debug_print_statement) {
            if (debug_print_statement.expression_) {
              DebugPrintValue(EvaluateExpression(*debug_print_statement.expression_));
            }
            return ControlFlow::kNormal;
          },
          [this](const pl_ast::LetStatement& let_statement) {
            runtime_state_.DeclareVariable(let_statement.identifier_.name_, EvaluateExpression(let_statement.initializer_expression_));
            return ControlFlow::kNormal;
          },
          [this](const pl_ast::AssignmentStatement& assignment_statement) {
            runtime_state_.AssignVariable(
              assignment_statement.identifier_.name_, EvaluateExpression(assignment_statement.assigned_expression_)
            );
            return ControlFlow::kNormal;
          },
          [this](const pl_ast::IfStatement& if_statement) {
            if (IsTruthy(EvaluateExpression(if_statement.if_condition_))) {
              return ExecuteBlock(if_statement.if_block_);
            }

            for (const auto& [else_if_condition, else_if_block] : if_statement.else_if_branches_) {
              if (IsTruthy(EvaluateExpression(else_if_condition))) {
                return ExecuteBlock(else_if_block);
              }
            }

            if (if_statement.else_block_) {
              return ExecuteBlock(*if_statement.else_block_);
            }
            return ControlFlow::kNormal;
          },
          [this](const pl_ast::WhileStatement& while_statement) {
            while (IsTruthy(EvaluateExpression(while_statement.while_condition_))) {
              const auto control_flow = ExecuteBlock(while_statement.while_block_);

              if (control_flow == ControlFlow::kBreak) {
                break;
              }
            }
            return ControlFlow::kNormal;
          },
          [this](const pl_ast::ForStatement& for_statement) {
            // Must add scope for initializer variable (needs to live past loop iterations).
            RuntimeState::ScopeGuard initializer_variable_scope(runtime_state_);
            ExecuteStatement(for_statement.initializer_);

            while (IsTruthy(EvaluateExpression(for_statement.condition_))) {
              const auto control_flow = ExecuteBlock(for_statement.for_block_);

              if (control_flow == ControlFlow::kBreak) {
                break;
              }

              ExecuteStatement(for_statement.update_);
            }
            return ControlFlow::kNormal;
          },
          [](const pl_ast::ContinueStatement&) { return ControlFlow::kContinue; },
          [](const pl_ast::BreakStatement&) { return ControlFlow::kBreak; },
          [this](const pl_ast::FunctionCallStatement& function_call_statement) {
            if (!std::holds_alternative<NothingValue>(EvaluateExpression(function_call_statement.function_call_))) {
              throw std::runtime_error("ExecuteStatement: function call didn't return nothing");
            }
            return ControlFlow::kNormal;
          },
          [this](const pl_ast::BlockPointer& block_pointer) { return ExecuteBlock(block_pointer); },
        },
        statement_variant
      );
    }

    Value EvaluateExpression(const pl_ast::ExpressionVariant& expression_variant) {
      return std::visit(
        template_helpers::Overloaded{
          [](const pl_ast::IntegerLiteralExpression& integer_expression) -> Value { return integer_expression.value_; },
          [](const pl_ast::BoolLiteralExpression& bool_expression) -> Value { return bool_expression.value_; },
          [](const pl_ast::NothingLiteralExpression&) -> Value { return NothingValue{}; },
          [this](const pl_ast::IdentifierExpression& identifier_expression) -> Value {
            return runtime_state_.LookupVariable(identifier_expression.identifier_.name_);
          },
          [this](const pl_ast::ArithmeticExpression& arithmetic_expression) -> Value {
            const auto [left_value, right_value] =
              EvaluateBothOperandsAsIntegers(arithmetic_expression.left_operand_, arithmetic_expression.right_operand_);
            return ExecuteArithmeticOperation(left_value, arithmetic_expression.operator_, right_value);
          },
          [this](const pl_ast::RelationalExpression& relational_expression) -> Value {
            const auto [left_value, right_value] =
              EvaluateBothOperandsAsIntegers(relational_expression.left_operand_, relational_expression.right_operand_);
            return ExecuteRelationalOperation(left_value, relational_expression.operator_, right_value);
          },
          [this](const pl_ast::EqualityExpression& equality_expression) -> Value {
            const auto [left_value, right_value] =
              EvaluateBothOperands(equality_expression.left_operand_, equality_expression.right_operand_);
            return ExecuteEqualityOperation(left_value, equality_expression.operator_, right_value);
          },
          [this](const pl_ast::LogicalExpression& logical_expression) -> Value {
            const auto left_value = EvaluateExpression(*logical_expression.left_operand_);

            if (logical_expression.operator_ == pl_ast::LogicalOperator::kAnd && !IsTruthy(left_value)) {
              return false;
            }
            if (logical_expression.operator_ == pl_ast::LogicalOperator::kOr && IsTruthy(left_value)) {
              return true;
            }

            return ExecuteLogicalOperation(
              IsTruthy(left_value), logical_expression.operator_, IsTruthy(EvaluateExpression(*logical_expression.right_operand_))
            );
          },
          [this](const pl_ast::FunctionCallExpression& function_call_expression) -> Value {
            return ExecuteFunction(runtime_state_.LookupFunctionOrThrow(function_call_expression.function_name_.name_));
          },
        },
        expression_variant
      );
    }
  };

  void ExecuteAstWithTreeInterpreter(const pl_ast::Program& program) {
    Interpreter interpreter;
    interpreter.Run(program);
  }
} // namespace tree_interpreter

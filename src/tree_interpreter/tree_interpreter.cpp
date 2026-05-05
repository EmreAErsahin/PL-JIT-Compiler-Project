#include <algorithm>
#include <iostream>
#include <iterator>
#include <optional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

#include "../common/overloaded.h"
#include "runtime_state.h"
#include "runtime_value.h"
#include "tree_interpreter.h"

namespace tree_interpreter {
  enum class ControlFlow { kNormal, kReturn, kContinue, kBreak };

  struct ExecutionResult {
    ControlFlow control_flow_ = ControlFlow::kNormal;
    std::optional<RuntimeValue> return_value_ = std::nullopt;
  };

  class Interpreter {
   public:
    void Run(const ast::Program& program) {
      const ast::Function& main_function = runtime_state_.BuildFunctionTableAndRequireMain(program);
      ExecuteFunction(main_function, {});
    }

   private:
    RuntimeState runtime_state_;

    std::pair<RuntimeValue, RuntimeValue>
    EvaluateBothOperands(const ast::ExpressionPointer& left_operand, const ast::ExpressionPointer& right_operand) {
      return {EvaluateExpression(*left_operand), EvaluateExpression(*right_operand)};
    }

    std::pair<RuntimeValue, RuntimeValue>
    EvaluateBothOperandsAsNumerics(const ast::ExpressionPointer& left_operand, const ast::ExpressionPointer& right_operand) {
      const auto [left_value, right_value] = EvaluateBothOperands(left_operand, right_operand);

      // Arithmetic and relational operators accept only numeric operands.
      ValidateNumericValue(left_value);
      ValidateNumericValue(right_value);

      return {left_value, right_value};
    }

    RuntimeValue ExecuteFunction(const ast::Function& function, std::span<const RuntimeValue> arguments) {
      if (arguments.size() != function.parameters_.size()) {
        throw std::runtime_error("ExecuteFunction: argument count doesn't match parameter count");
      }

      RuntimeState::CallFrameGuard call_frame_guard(runtime_state_);
      RuntimeState::ScopeGuard function_level_scope(runtime_state_);
      // Parameters are ordinary variables in the function's top-level lexical scope.
      for (auto&& [parameter, argument] : std::views::zip(function.parameters_, arguments)) {
        runtime_state_.DeclareVariable(parameter.name_, argument);
      }

      const auto [control_flow, return_value] = ExecuteBlock(function.function_block_, BlockScope::kReuseScope);

      switch (control_flow) {
        case ControlFlow::kNormal: return NothingValue{};
        case ControlFlow::kReturn: return return_value ? *return_value : NothingValue{};
        case ControlFlow::kContinue: throw std::runtime_error("ExecuteFunction: continue statement not within a loop");
        case ControlFlow::kBreak: throw std::runtime_error("ExecuteFunction: break statement not within a loop");
      }

      throw std::runtime_error("ExecuteFunction: unknown control flow");
    }

    ExecutionResult ExecuteStatementsInBlock(const ast::BlockPointer& block_pointer) {
      ExecutionResult execution_result;
      for (const auto& statement_variant : block_pointer->statements_) {
        execution_result = ExecuteStatement(statement_variant);
        if (execution_result.control_flow_ != ControlFlow::kNormal) {
          break;
        }
      }
      return execution_result;
    }

    enum class BlockScope { kCreateScope, kReuseScope };

    ExecutionResult ExecuteBlock(const ast::BlockPointer& block_pointer, const BlockScope block_scope_behavior = BlockScope::kCreateScope) {
      if (!block_pointer) {
        throw std::runtime_error("ExecuteBlock: null block pointer");
      }

      // Function bodies reuse the scope created for parameters; nested blocks create their own lexical scope.
      if (block_scope_behavior == BlockScope::kCreateScope) {
        RuntimeState::ScopeGuard block_scope(runtime_state_);

        return ExecuteStatementsInBlock(block_pointer);
      }
      return ExecuteStatementsInBlock(block_pointer);
    }

    ExecutionResult ExecuteStatement(const ast::StatementVariant& statement_variant) {
      return std::visit(
        template_helpers::Overloaded{
          [this](const ast::PrintStatement& print_statement) {
            if (print_statement.print_expression_) {
              PrintValue(EvaluateExpression(*print_statement.print_expression_));
            }
            if (print_statement.new_line_) {
              std::cout << "\n";
            }
            return ExecutionResult{ControlFlow::kNormal};
          },
          [this](const ast::LetStatement& let_statement) {
            runtime_state_.DeclareVariable(let_statement.identifier_.name_, EvaluateExpression(let_statement.initializer_expression_));
            return ExecutionResult{ControlFlow::kNormal};
          },
          [this](const ast::AssignmentStatement& assignment_statement) {
            runtime_state_.AssignVariable(
              assignment_statement.identifier_.name_, EvaluateExpression(assignment_statement.assigned_expression_)
            );
            return ExecutionResult{ControlFlow::kNormal};
          },
          [this](const ast::IfStatement& if_statement) {
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
            return ExecutionResult{ControlFlow::kNormal};
          },
          [this](const ast::WhileStatement& while_statement) {
            while (IsTruthy(EvaluateExpression(while_statement.while_condition_))) {
              const ExecutionResult execution_result = ExecuteBlock(while_statement.while_block_);

              switch (execution_result.control_flow_) {
                case ControlFlow::kNormal:
                case ControlFlow::kContinue: continue;
                case ControlFlow::kBreak: return ExecutionResult{ControlFlow::kNormal};
                case ControlFlow::kReturn: return execution_result;
              }
            }
            return ExecutionResult{ControlFlow::kNormal};
          },
          [this](const ast::ForStatement& for_statement) {
            // Must add scope for initializer variable (needs to live past loop iterations).
            RuntimeState::ScopeGuard initializer_variable_scope(runtime_state_);
            ExecuteStatement(for_statement.initializer_);

            while (IsTruthy(EvaluateExpression(for_statement.condition_))) {
              const ExecutionResult execution_result = ExecuteBlock(for_statement.for_block_);

              switch (execution_result.control_flow_) {
                case ControlFlow::kBreak: return ExecutionResult{ControlFlow::kNormal};
                case ControlFlow::kReturn: return execution_result;
                case ControlFlow::kNormal:
                case ControlFlow::kContinue: ExecuteStatement(for_statement.update_);
              }
            }
            return ExecutionResult{ControlFlow::kNormal};
          },
          [](const ast::ContinueStatement&) { return ExecutionResult{ControlFlow::kContinue}; },
          [](const ast::BreakStatement&) { return ExecutionResult{ControlFlow::kBreak}; },
          [this](const ast::ReturnStatement& return_statement) {
            return ExecutionResult{
              ControlFlow::kReturn,
              return_statement.return_expression_ ? EvaluateExpression(*return_statement.return_expression_) : NothingValue{}
            };
          },
          [this](const ast::FunctionCallStatement& function_call_statement) {
            EvaluateExpression(function_call_statement.function_call_);
            // Function call statements discard return values and run only for side effects.
            return ExecutionResult{ControlFlow::kNormal};
          },
          [this](const ast::BlockPointer& block_pointer) { return ExecuteBlock(block_pointer); },
        },
        statement_variant
      );
    }

    RuntimeValue EvaluateExpression(const ast::ExpressionVariant& expression_variant) {
      return std::visit(
        template_helpers::Overloaded{
          [](const ast::IntegerLiteralExpression& integer_expression) -> RuntimeValue { return integer_expression.value_; },
          [](const ast::DoubleLiteralExpression& double_expression) -> RuntimeValue { return double_expression.value_; },
          [](const ast::BoolLiteralExpression& bool_expression) -> RuntimeValue { return bool_expression.value_; },
          [](const ast::StringLiteralExpression& string_expression) -> RuntimeValue { return string_expression.value_; },
          [](const ast::NothingLiteralExpression&) -> RuntimeValue { return NothingValue{}; },
          [this](const ast::IdentifierExpression& identifier_expression) -> RuntimeValue {
            return runtime_state_.LookupVariable(identifier_expression.identifier_.name_);
          },
          [this](const ast::ArithmeticExpression& arithmetic_expression) -> RuntimeValue {
            const auto [left_value, right_value] =
              EvaluateBothOperandsAsNumerics(arithmetic_expression.left_operand_, arithmetic_expression.right_operand_);

            return ExecuteArithmeticOperation(left_value, arithmetic_expression.operator_, right_value);
          },
          [this](const ast::RelationalExpression& relational_expression) -> RuntimeValue {
            const auto [left_value, right_value] =
              EvaluateBothOperandsAsNumerics(relational_expression.left_operand_, relational_expression.right_operand_);
            return ExecuteRelationalOperation(left_value, relational_expression.operator_, right_value);
          },
          [this](const ast::EqualityExpression& equality_expression) -> RuntimeValue {
            const auto [left_value, right_value] =
              EvaluateBothOperands(equality_expression.left_operand_, equality_expression.right_operand_);
            return ExecuteEqualityOperation(left_value, equality_expression.operator_, right_value);
          },
          [this](const ast::LogicalExpression& logical_expression) -> RuntimeValue {
            const bool left_truthiness = IsTruthy(EvaluateExpression(*logical_expression.left_operand_));

            // Preserve short-circuiting so the right operand is evaluated only when needed.
            if (logical_expression.operator_ == ast::LogicalOperator::kAnd && !left_truthiness) {
              return false;
            }
            if (logical_expression.operator_ == ast::LogicalOperator::kOr && left_truthiness) {
              return true;
            }

            return ExecuteLogicalOperation(
              left_truthiness, logical_expression.operator_, IsTruthy(EvaluateExpression(*logical_expression.right_operand_))
            );
          },
          [this](const ast::UnaryExpression& unary_expression) -> RuntimeValue {
            return ExecuteUnaryOperation(unary_expression.operator_, EvaluateExpression(*unary_expression.operand_));
          },
          [this](const ast::FunctionCallExpression& function_call_expression) -> RuntimeValue {
            std::vector<RuntimeValue> evaluated_arguments;
            evaluated_arguments.reserve(function_call_expression.arguments_.size());

            std::ranges::transform(
              function_call_expression.arguments_, std::back_inserter(evaluated_arguments),
              [this](const auto& argument) { return EvaluateExpression(*argument); }
            );

            return ExecuteFunction(
              runtime_state_.LookupFunctionOrThrow(function_call_expression.function_name_.name_), evaluated_arguments
            );
          },
        },
        expression_variant
      );
    }
  };

  void ExecuteAstWithTreeInterpreter(const ast::Program& program) {
    Interpreter interpreter;
    interpreter.Run(program);
  }
} // namespace tree_interpreter

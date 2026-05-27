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
  Interpreter::Interpreter(const ast::Program& program) {
    runtime_state_.BuildFunctionTable(program);
  }

  RuntimeValue Interpreter::ExecuteFunction(const std::string& function_name, std::span<const RuntimeValue> arguments) {
    const ast::Function& function = runtime_state_.LookupFunctionOrThrow(function_name);

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

  std::pair<RuntimeValue, RuntimeValue>
  Interpreter::EvaluateBothOperands(const ast::ExpressionPointer& left_operand, const ast::ExpressionPointer& right_operand) {
    return {EvaluateExpression(*left_operand), EvaluateExpression(*right_operand)};
  }

  std::pair<RuntimeValue, RuntimeValue>
  Interpreter::EvaluateBothOperandsAsNumerics(const ast::ExpressionPointer& left_operand, const ast::ExpressionPointer& right_operand) {
    const auto [left_value, right_value] = EvaluateBothOperands(left_operand, right_operand);

    // Arithmetic and relational operators accept only numeric operands.
    ValidateNumericValue(left_value);
    ValidateNumericValue(right_value);

    return {left_value, right_value};
  }

  ExecutionResult Interpreter::ExecuteStatementsInBlock(const ast::BlockPointer& block_pointer) {
    ExecutionResult execution_result;
    for (const auto& statement_variant : block_pointer->statements_) {
      execution_result = ExecuteStatement(statement_variant);
      if (execution_result.control_flow_ != ControlFlow::kNormal) {
        break;
      }
    }
    return execution_result;
  }

  ExecutionResult Interpreter::ExecuteBlock(const ast::BlockPointer& block_pointer, const BlockScope block_scope_behavior) {
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

  IndexedSlotLocation Interpreter::LocateIndexedSlot(const ast::IndexExpression& index_expression) {
    RuntimeValue current_array = EvaluateExpression(*index_expression.indexed_expression_);

    const size_t index_expression_count = index_expression.indexing_expressions_.size();
    for (size_t index_expression_index = 0; index_expression_index < index_expression_count; ++index_expression_index) {
      const auto* current_array_pointer = std::get_if<RuntimeArrayPointer>(&current_array);
      if (!current_array_pointer || !*current_array_pointer) {
        throw std::runtime_error("LocateIndexedSlot: cannot index into a non-array value");
      }

      const RuntimeValue index_value = EvaluateExpression(*index_expression.indexing_expressions_[index_expression_index]);
      const auto* index_pointer = std::get_if<int64_t>(&index_value);
      if (!index_pointer) {
        throw std::runtime_error("LocateIndexedSlot: index must be an integer");
      }
      if (*index_pointer < 0 || static_cast<size_t>(*index_pointer) >= (*current_array_pointer)->elements_.size()) {
        throw std::runtime_error("LocateIndexedSlot: index " + std::to_string(*index_pointer) + " out of bounds");
      }

      const auto element_index = static_cast<size_t>(*index_pointer);
      if (index_expression_index == index_expression_count - 1) {
        return IndexedSlotLocation{.array_ = *current_array_pointer, .index_ = element_index};
      }

      current_array = (*current_array_pointer)->elements_[element_index];
    }

    throw std::runtime_error("LocateIndexedSlot: index expression has no indexing expressions");
  }

  ExecutionResult Interpreter::ExecuteStatement(const ast::StatementVariant& statement_variant) {
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
        [this](const ast::PushStatement& push_statement) {
          EvaluateExpression(push_statement.push_expression_);
          return ExecutionResult{ControlFlow::kNormal};
        },
        [this](const ast::PopStatement& pop_statement) {
          EvaluateExpression(pop_statement.pop_expression_);
          return ExecutionResult{ControlFlow::kNormal};
        },
        [this](const ast::IndexAssignmentStatement& index_assignment_statement) {
          const auto [final_array_pointer, final_index] = LocateIndexedSlot(index_assignment_statement.target_);

          final_array_pointer->elements_[final_index] = EvaluateExpression(*index_assignment_statement.assigned_expression_);

          return ExecutionResult{ControlFlow::kNormal};
        },
        [this](const ast::BlockPointer& block_pointer) { return ExecuteBlock(block_pointer); },
      },
      statement_variant
    );
  }

  RuntimeValue Interpreter::EvaluateExpression(const ast::ExpressionVariant& expression_variant) {
    return std::visit(
      template_helpers::Overloaded{
        [](const ast::IntegerLiteralExpression& integer_expression) -> RuntimeValue { return integer_expression.value_; },
        [](const ast::DoubleLiteralExpression& double_expression) -> RuntimeValue { return double_expression.value_; },
        [](const ast::BoolLiteralExpression& bool_expression) -> RuntimeValue { return bool_expression.value_; },
        [](const ast::StringLiteralExpression& string_expression) -> RuntimeValue { return string_expression.value_; },
        [this](const ast::ArrayLiteralExpression& array_expression) -> RuntimeValue {
          std::vector<RuntimeValue> evaluated_array_elements;
          evaluated_array_elements.reserve(array_expression.elements_.size());

          std::ranges::transform(array_expression.elements_, std::back_inserter(evaluated_array_elements), [this](const auto& element) {
            return EvaluateExpression(*element);
          });

          return std::make_shared<RuntimeArray>(RuntimeArray{std::move(evaluated_array_elements)});
        },
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
        [this](const ast::LengthExpression& length_expression) -> RuntimeValue {
          const RuntimeValue length_operand = EvaluateExpression(*length_expression.expression_);
          return std::visit(
            template_helpers::Overloaded{
              [](const std::string& string_operand) -> RuntimeValue { return static_cast<int64_t>(string_operand.size()); },
              [](const RuntimeArrayPointer& array_operand) -> RuntimeValue {
                return static_cast<int64_t>(array_operand->elements_.size());
              },
              [](const auto&) -> RuntimeValue { throw std::runtime_error("EvaluateExpression: can only take length of string or array"); }
            },
            length_operand
          );
        },
        [this](const ast::PushExpression& push_expression) -> RuntimeValue {
          const RuntimeValue target_value = EvaluateExpression(*push_expression.target_);
          const auto* target_array_pointer = std::get_if<RuntimeArrayPointer>(&target_value);
          if (!target_array_pointer || !*target_array_pointer) {
            throw std::runtime_error("EvaluateExpression: can only push to an array");
          }

          const RuntimeValue pushed_value = EvaluateExpression(*push_expression.pushed_expression_);
          (*target_array_pointer)->elements_.push_back(pushed_value);

          return NothingValue{};
        },
        [this](const ast::PopExpression& pop_expression) -> RuntimeValue {
          const RuntimeValue target_value = EvaluateExpression(*pop_expression.target_);
          const auto* target_array_pointer = std::get_if<RuntimeArrayPointer>(&target_value);
          if (!target_array_pointer || !*target_array_pointer) {
            throw std::runtime_error("EvaluateExpression: can only pop from an array");
          }

          if ((*target_array_pointer)->elements_.empty()) {
            throw std::runtime_error("EvaluateExpression: cannot pop from an empty array");
          }

          const RuntimeValue popped_value = (*target_array_pointer)->elements_.back();
          (*target_array_pointer)->elements_.pop_back();

          return popped_value;
        },
        [this](const ast::FunctionCallExpression& function_call_expression) -> RuntimeValue {
          std::vector<RuntimeValue> evaluated_arguments;
          evaluated_arguments.reserve(function_call_expression.arguments_.size());

          std::ranges::transform(
            function_call_expression.arguments_, std::back_inserter(evaluated_arguments),
            [this](const auto& argument) { return EvaluateExpression(*argument); }
          );

          return ExecuteFunction(function_call_expression.function_name_.name_, evaluated_arguments);
        },
        [this](const ast::IndexExpression& index_expression) -> RuntimeValue {
          const auto [final_array_pointer, final_index] = LocateIndexedSlot(index_expression);
          return final_array_pointer->elements_[final_index];
        },
      },
      expression_variant
    );
  }
} // namespace tree_interpreter

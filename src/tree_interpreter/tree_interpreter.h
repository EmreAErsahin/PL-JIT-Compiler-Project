#ifndef tree_interpreter_H
#define tree_interpreter_H

#include <cstddef>
#include <optional>
#include <span>
#include <utility>

#include "../ast/ast.h"
#include "runtime_state.h"
#include "runtime_value.h"

namespace tree_interpreter {
  enum class ControlFlow { kNormal, kReturn, kContinue, kBreak };

  struct ExecutionResult {
    ControlFlow control_flow_ = ControlFlow::kNormal;
    std::optional<RuntimeValue> return_value_ = std::nullopt;
  };

  enum class BlockScope { kCreateScope, kReuseScope };

  struct IndexedSlotLocation {
    RuntimeArrayPointer array_;
    size_t index_;
  };

  class Interpreter {
   public:
    explicit Interpreter(const ast::Program& program);

    RuntimeValue ExecuteFunction(const std::string& function_name, std::span<const RuntimeValue> arguments);

   private:
    RuntimeState runtime_state_;

    std::pair<RuntimeValue, RuntimeValue>
    EvaluateBothOperands(const ast::ExpressionPointer& left_operand, const ast::ExpressionPointer& right_operand);

    std::pair<RuntimeValue, RuntimeValue>
    EvaluateBothOperandsAsNumerics(const ast::ExpressionPointer& left_operand, const ast::ExpressionPointer& right_operand);

    ExecutionResult ExecuteStatementsInBlock(const ast::BlockPointer& block_pointer);

    ExecutionResult ExecuteBlock(const ast::BlockPointer& block_pointer, BlockScope block_scope_behavior = BlockScope::kCreateScope);

    IndexedSlotLocation LocateIndexedSlot(const ast::IndexExpression& index_expression);

    ExecutionResult ExecuteStatement(const ast::StatementVariant& statement_variant);

    RuntimeValue EvaluateExpression(const ast::ExpressionVariant& expression_variant);
  };
} // namespace tree_interpreter

#endif // tree_interpreter_H

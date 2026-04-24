#ifndef pl_ast_H
#define pl_ast_H

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace pl_ast {
  struct IntegerLiteralExpression {
    int64_t value_;
  };

  struct BoolLiteralExpression {
    bool value_;
  };

  struct NothingLiteralExpression {};

  enum class ArithmeticOperator { kAdd, kSubtract, kMultiply, kDivide };

  enum class RelationalOperator { kLessThan, kLessThanOrEqual, kGreaterThan, kGreaterThanOrEqual };

  enum class EqualityOperator { kEqual, kNotEqual };

  enum class LogicalOperator { kAnd, kOr };

  // Variables, function names
  struct Identifier {
    std::string name_;
  };

  struct IdentifierExpression {
    Identifier identifier_;
  };

  // clang-format off
  struct ArithmeticExpression;
  struct RelationalExpression;
  struct EqualityExpression;
  struct LogicalExpression;
  // clang-format on

  struct FunctionCallExpression {
    Identifier function_name_;
  };

  using ExpressionVariant = std::variant<
    IntegerLiteralExpression, BoolLiteralExpression, NothingLiteralExpression, IdentifierExpression, ArithmeticExpression,
    RelationalExpression, EqualityExpression, LogicalExpression, FunctionCallExpression>;

  // TODO: Try to find a better alternative for shared ptr. Shared ownership is awkward bc only the AST needs
  // to own this, but cpp-peglib stores semantic values as std::any which requires copyable objects
  using CopyableExpressionPointer = std::shared_ptr<ExpressionVariant>;

  struct ArithmeticExpression {
    CopyableExpressionPointer left_operand_;
    ArithmeticOperator operator_;
    CopyableExpressionPointer right_operand_;
  };

  struct RelationalExpression {
    CopyableExpressionPointer left_operand_;
    RelationalOperator operator_;
    CopyableExpressionPointer right_operand_;
  };

  struct EqualityExpression {
    CopyableExpressionPointer left_operand_;
    EqualityOperator operator_;
    CopyableExpressionPointer right_operand_;
  };

  struct LogicalExpression {
    CopyableExpressionPointer left_operand_;
    LogicalOperator operator_;
    CopyableExpressionPointer right_operand_;
  };

  struct PrintStatement {
    std::optional<ExpressionVariant> expression_;
  };

  // Bare declarations are not supported
  struct LetStatement {
    Identifier identifier_;
    ExpressionVariant initializer_expression_;
  };

  struct AssignmentStatement {
    Identifier identifier_;
    ExpressionVariant assigned_expression_;
  };

  // We need to add Blocks as a statement variant, so blocks can be nested
  struct Block;
  // Remember: cpp-peglib requires this to be a copyable type
  using BlockPointer = std::shared_ptr<Block>;

  using ElseIfConditionBlockPairs = std::vector<std::pair<ExpressionVariant, BlockPointer>>;

  struct IfStatement {
    ExpressionVariant if_condition_;
    BlockPointer if_block_;
    ElseIfConditionBlockPairs else_if_branches_;
    std::optional<BlockPointer> else_block_;
  };

  struct WhileStatement {
    ExpressionVariant while_condition_;
    BlockPointer while_block_;
  };

  struct ForStatement {
    LetStatement initializer_;
    ExpressionVariant condition_;
    AssignmentStatement update_;
    BlockPointer for_block_;
  };

  struct ContinueStatement {};

  struct BreakStatement {};

  struct ReturnStatement {
    std::optional<ExpressionVariant> return_expression_;
  };

  struct FunctionCallStatement {
    FunctionCallExpression function_call_;
  };

  using StatementVariant = std::variant<
    PrintStatement, LetStatement, AssignmentStatement, BlockPointer, IfStatement, WhileStatement, ForStatement, ContinueStatement,
    BreakStatement, ReturnStatement, FunctionCallStatement>;

  struct Block {
    std::vector<StatementVariant> statements_;
  };

  struct Function {
    Identifier identifier_;
    // Makes parsing actions more simple to store pointer here
    BlockPointer function_block_;
  };

  struct Program {
    std::vector<Function> functions_;
  };
} // namespace pl_ast

#endif // pl_ast_H

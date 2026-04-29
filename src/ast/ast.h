#ifndef ast_H
#define ast_H

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace ast {
  struct IntegerLiteralExpression {
    int64_t value_;
  };

  struct BoolLiteralExpression {
    bool value_;
  };

  struct NothingLiteralExpression {};

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
  struct UnaryExpression;
  struct FunctionCallExpression;
  // clang-format on

  using ExpressionVariant = std::variant<
    IntegerLiteralExpression, BoolLiteralExpression, NothingLiteralExpression, IdentifierExpression, ArithmeticExpression,
    RelationalExpression, EqualityExpression, LogicalExpression, UnaryExpression, FunctionCallExpression>;

  // TODO: Try to find a better alternative for shared ptr. Shared ownership is awkward bc only the AST needs
  // to own this, but cpp-peglib stores semantic values as std::any which requires copyable objects
  using CopyableExpressionPointer = std::shared_ptr<ExpressionVariant>;

  enum class ArithmeticOperator { kAdd, kSubtract, kMultiply, kDivide };

  struct ArithmeticExpression {
    CopyableExpressionPointer left_operand_;
    ArithmeticOperator operator_;
    CopyableExpressionPointer right_operand_;
  };

  enum class RelationalOperator { kLessThan, kLessThanOrEqual, kGreaterThan, kGreaterThanOrEqual };

  struct RelationalExpression {
    CopyableExpressionPointer left_operand_;
    RelationalOperator operator_;
    CopyableExpressionPointer right_operand_;
  };

  enum class EqualityOperator { kEqual, kNotEqual };

  struct EqualityExpression {
    CopyableExpressionPointer left_operand_;
    EqualityOperator operator_;
    CopyableExpressionPointer right_operand_;
  };

  enum class LogicalOperator { kAnd, kOr };

  struct LogicalExpression {
    CopyableExpressionPointer left_operand_;
    LogicalOperator operator_;
    CopyableExpressionPointer right_operand_;
  };

  enum class UnaryOperator { kNegate, kNot };

  struct UnaryExpression {
    UnaryOperator operator_;
    CopyableExpressionPointer operand_;
  };

  struct FunctionCallExpression {
    Identifier function_name_;
    std::vector<CopyableExpressionPointer> arguments_;
  };

  // supports both print and println
  struct PrintStatement {
    std::optional<ExpressionVariant> print_expression_;
    bool new_line_;
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
    std::vector<Identifier> parameters_;
    // Makes parsing actions more simple to store pointer here
    BlockPointer function_block_;
  };

  struct Program {
    std::vector<Function> functions_;
  };
} // namespace ast

#endif // ast_H

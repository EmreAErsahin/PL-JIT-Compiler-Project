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

  struct DoubleLiteralExpression {
    double value_;
  };

  struct BoolLiteralExpression {
    bool value_;
  };

  struct StringLiteralExpression {
    std::string value_;
  };

  struct NothingLiteralExpression {};

  // Identifiers name variables, functions, and parameters.
  struct Identifier {
    std::string name_;
  };

  struct IdentifierExpression {
    Identifier identifier_;
  };

  // clang-format off
  struct ArrayLiteralExpression;
  struct ArithmeticExpression;
  struct RelationalExpression;
  struct EqualityExpression;
  struct LogicalExpression;
  struct UnaryExpression;
  struct FunctionCallExpression;
  struct IndexExpression;
  // clang-format on

  using ExpressionVariant = std::variant<
    IntegerLiteralExpression, DoubleLiteralExpression, BoolLiteralExpression, StringLiteralExpression, ArrayLiteralExpression,
    NothingLiteralExpression, IdentifierExpression, ArithmeticExpression, RelationalExpression, EqualityExpression, LogicalExpression,
    UnaryExpression, FunctionCallExpression, IndexExpression>;

  // cpp-peglib stores semantic values as std::any, so recursive AST nodes must be copyable during parsing.
  // The AST still owns these nodes logically; shared_ptr is used for parser compatibility.
  using ExpressionPointer = std::shared_ptr<ExpressionVariant>;

  struct ArrayLiteralExpression {
    std::vector<ExpressionPointer> elements_;
  };

  enum class ArithmeticOperator { kAdd, kSubtract, kMultiply, kDivide, kModulo };

  struct ArithmeticExpression {
    ExpressionPointer left_operand_;
    ArithmeticOperator operator_;
    ExpressionPointer right_operand_;
  };

  enum class RelationalOperator { kLessThan, kLessThanOrEqual, kGreaterThan, kGreaterThanOrEqual };

  struct RelationalExpression {
    ExpressionPointer left_operand_;
    RelationalOperator operator_;
    ExpressionPointer right_operand_;
  };

  enum class EqualityOperator { kEqual, kNotEqual };

  struct EqualityExpression {
    ExpressionPointer left_operand_;
    EqualityOperator operator_;
    ExpressionPointer right_operand_;
  };

  enum class LogicalOperator { kAnd, kOr };

  struct LogicalExpression {
    ExpressionPointer left_operand_;
    LogicalOperator operator_;
    ExpressionPointer right_operand_;
  };

  enum class UnaryOperator { kNegate, kNot };

  struct UnaryExpression {
    UnaryOperator operator_;
    ExpressionPointer operand_;
  };

  struct FunctionCallExpression {
    Identifier function_name_;
    std::vector<ExpressionPointer> arguments_;
  };

  struct IndexExpression {
    ExpressionPointer indexed_expression_;
    std::vector<ExpressionPointer> indexing_expressions_;
  };

  // Shared node for print and println; new_line_ decides whether to append '\n'.
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

  struct Block;
  // Nested blocks also need copyable handles for parser semantic values.
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

  struct IndexAssignmentStatement {
    IndexExpression target_;
    ExpressionPointer assigned_expression_;
  };

  using StatementVariant = std::variant<
    PrintStatement, LetStatement, AssignmentStatement, BlockPointer, IfStatement, WhileStatement, ForStatement, ContinueStatement,
    BreakStatement, ReturnStatement, FunctionCallStatement, IndexAssignmentStatement>;

  struct Block {
    std::vector<StatementVariant> statements_;
  };

  struct Function {
    Identifier identifier_;
    std::vector<Identifier> parameters_;
    // Reuses BlockPointer because cpp-peglib semantic values must stay copyable.
    BlockPointer function_block_;
  };

  struct Program {
    std::vector<Function> functions_;
  };
} // namespace ast

#endif // ast_H

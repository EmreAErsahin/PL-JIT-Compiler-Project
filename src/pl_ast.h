#ifndef pl_ast_H
#define pl_ast_H

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
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

  enum class ArithmeticOperator { ADD, SUBTRACT, MULTIPLY, DIVIDE };

  // Variables, function names
  struct Identifier {
    std::string name_;
  };

  struct IdentifierExpression {
    Identifier identifier_;
  };

  // Forward declaration bc of circular dependency with BinaryExpression & ExpressionVariant
  struct BinaryExpression;

  using ExpressionVariant =
    std::variant<IntegerLiteralExpression, BoolLiteralExpression, NothingLiteralExpression, BinaryExpression, IdentifierExpression>;

  // TODO: Try to find a better alternative for shared ptr. Shared ownership is awkward bc only the AST needs
  // to own this, but cpp-peglib stores semantic values as std::any which requires copyable objects
  using ExpressionPointer = std::shared_ptr<ExpressionVariant>;

  struct BinaryExpression {
    ExpressionPointer left_operand_;
    ArithmeticOperator operator_;
    ExpressionPointer right_operand_;
  };

  struct DebugPrintStatement {
    std::optional<ExpressionVariant> expression_;
  };

  // Variable bindings are introduced with an initializer expression; bare declarations are not supported.
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

  using StatementVariant = std::variant<DebugPrintStatement, LetStatement, AssignmentStatement, BlockPointer>;

  struct Block {
    std::vector<StatementVariant> statements_;
  };

  struct Function {
    Identifier identifier_;
    // Makes parsing actions more simple to store pointer here
    BlockPointer function_block_;
  };

  // TODO: Change to a vector of functions once we allow multiple
  struct Program {
    Function function_;
  };
} // namespace pl_ast

#endif // pl_ast_H

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

  // Forward declaration bc of circular dependency with BinaryExpression & ExpressionVariant
  struct BinaryExpression;

  using ExpressionVariant = std::variant<IntegerLiteralExpression, BoolLiteralExpression, NothingLiteralExpression, BinaryExpression>;

  // TODO: Try to find a better alternative. Shared ownership is awkward cuz only the AST needs
  // to own this, but peglib stores semantic values as std::any which requires copyable objects
  using ExpressionPointer = std::shared_ptr<ExpressionVariant>;

  struct BinaryExpression {
    ExpressionPointer left_operand_;
    ArithmeticOperator operator_;
    ExpressionPointer right_operand_;
  };

  struct DebugPrintStatement {
    std::optional<ExpressionVariant> expression_;
  };

  using StatementVariant = std::variant<DebugPrintStatement>;

  struct Function {
    Identifier identifier_;
    std::vector<StatementVariant> statements_;
  };

  // TODO: Change to a vector of functions once we allow multiple
  struct Program {
    Function function_;
  };
} // namespace pl_ast

#endif // pl_ast_H

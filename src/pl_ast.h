#ifndef pl_ast_H
#define pl_ast_H

#include <cstdint>
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

  using ExpressionVariant = std::variant<IntegerLiteralExpression, BoolLiteralExpression, NothingLiteralExpression>;

  // Variables, function names
  struct Identifier {
    std::string name_;
  };

  struct PrintStatement {
    std::optional<ExpressionVariant> expression_;
  };

  using StatementVariant = std::variant<PrintStatement>;

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
#ifndef runtime_value_H
#define runtime_value_H

#include <cstdint>
#include <string>
#include <variant>

#include "../ast/ast.h"

namespace tree_interpreter {
  struct NothingValue {};

  // Dynamic value model for evaluated expressions, plus operations that define value semantics.
  using RuntimeValue = std::variant<int64_t, double, bool, std::string, NothingValue>;

  bool IsTruthy(const RuntimeValue& value);

  void ValidateNumericValue(const RuntimeValue& value);

  RuntimeValue
  ExecuteArithmeticOperation(const RuntimeValue& left_value, ast::ArithmeticOperator arithmetic_operator, const RuntimeValue& right_value);

  RuntimeValue
  ExecuteRelationalOperation(const RuntimeValue& left_value, ast::RelationalOperator relational_operator, const RuntimeValue& right_value);

  RuntimeValue
  ExecuteEqualityOperation(const RuntimeValue& left_value, ast::EqualityOperator equality_operator, const RuntimeValue& right_value);

  RuntimeValue ExecuteLogicalOperation(bool left_value, ast::LogicalOperator logical_operator, bool right_value);

  RuntimeValue ExecuteUnaryOperation(ast::UnaryOperator unary_operator, const RuntimeValue& operand_value);

  void PrintValue(const RuntimeValue& value);
} // namespace tree_interpreter

#endif // runtime_value_H

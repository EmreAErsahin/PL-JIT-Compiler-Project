#include <concepts>
#include <iostream>
#include <stdexcept>

#include "../common/overloaded.h"
#include "runtime_value.h"

namespace tree_interpreter {
  bool IsTruthy(const RuntimeValue& value) {
    return std::visit(
      template_helpers::Overloaded{
        [](const int64_t integer_value) { return integer_value != 0; },
        [](const double double_value) { return double_value != 0; },
        [](const bool bool_value) { return bool_value; },
        [](const NothingValue&) { return false; },
      },
      value
    );
  }

  void ValidateNumericValue(const RuntimeValue& value) {
    if (!(std::holds_alternative<int64_t>(value) || std::holds_alternative<double>(value))) {
      throw std::runtime_error("ValidateNumericValue: value must be a double or integer");
    }
  }

  template <typename T>
  concept NumericRuntimeType = std::same_as<T, int64_t> || std::same_as<T, double>;

  template <NumericRuntimeType LeftOperand, NumericRuntimeType RightOperand>
  RuntimeValue ComputeArithmeticOperation(
    const LeftOperand left_operand, const ast::ArithmeticOperator arithmetic_operator, const RightOperand right_operand
  ) {
    switch (arithmetic_operator) {
      case ast::ArithmeticOperator::kAdd: return left_operand + right_operand;
      case ast::ArithmeticOperator::kSubtract: return left_operand - right_operand;
      case ast::ArithmeticOperator::kMultiply: return left_operand * right_operand;
      case ast::ArithmeticOperator::kDivide:
        if (right_operand == 0) {
          throw std::runtime_error("ExecuteArithmeticOperation: division by zero");
        }
        return left_operand / right_operand;
      case ast::ArithmeticOperator::kModulo: break;
    }

    throw std::runtime_error("ExecuteArithmeticOperation: unsupported arithmetic operator or modulo attempted with non-integers");
  }

  RuntimeValue ExecuteArithmeticOperation(
    const RuntimeValue& left_value, const ast::ArithmeticOperator arithmetic_operator, const RuntimeValue& right_value
  ) {
    return std::visit(
      template_helpers::Overloaded{
        [arithmetic_operator](const int64_t left_integer, const int64_t right_integer) -> RuntimeValue {
          // Modulo is the only arithmetic operation that requires integer operands.
          if (arithmetic_operator == ast::ArithmeticOperator::kModulo) {
            if (right_integer == 0) {
              throw std::runtime_error("ExecuteArithmeticOperation: modulo by zero");
            }
            return left_integer % right_integer;
          }

          return ComputeArithmeticOperation(left_integer, arithmetic_operator, right_integer);
        },
        [arithmetic_operator](const double left_double, const double right_double) -> RuntimeValue {
          return ComputeArithmeticOperation(left_double, arithmetic_operator, right_double);
        },
        [arithmetic_operator](const int64_t left_integer, const double right_double) -> RuntimeValue {
          return ComputeArithmeticOperation(left_integer, arithmetic_operator, right_double);
        },
        [arithmetic_operator](const double left_double, const int64_t right_integer) -> RuntimeValue {
          return ComputeArithmeticOperation(left_double, arithmetic_operator, right_integer);
        },
        [](const auto&, const auto&) -> RuntimeValue {
          throw std::runtime_error("ExecuteArithmeticOperation: unsupported arithmetic operator");
        },
      },
      left_value, right_value
    );
  }

  template <NumericRuntimeType LeftOperand, NumericRuntimeType RightOperand>
  RuntimeValue ComputeRelationalOperation(
    const LeftOperand left_operand, const ast::RelationalOperator relational_operator, const RightOperand right_operand
  ) {
    switch (relational_operator) {
      case ast::RelationalOperator::kLessThan: return left_operand < right_operand;
      case ast::RelationalOperator::kLessThanOrEqual: return left_operand <= right_operand;
      case ast::RelationalOperator::kGreaterThan: return left_operand > right_operand;
      case ast::RelationalOperator::kGreaterThanOrEqual: return left_operand >= right_operand;
    }

    throw std::runtime_error("ExecuteRelationalOperation: unsupported relational operator");
  }

  RuntimeValue ExecuteRelationalOperation(
    const RuntimeValue& left_value, const ast::RelationalOperator relational_operator, const RuntimeValue& right_value
  ) {
    return std::visit(
      template_helpers::Overloaded{
        [relational_operator](const int64_t left_integer, const int64_t right_integer) -> RuntimeValue {
          return ComputeRelationalOperation(left_integer, relational_operator, right_integer);
        },
        [relational_operator](const double left_double, const double right_double) -> RuntimeValue {
          return ComputeRelationalOperation(left_double, relational_operator, right_double);
        },
        [relational_operator](const int64_t left_integer, const double right_double) -> RuntimeValue {
          return ComputeRelationalOperation(left_integer, relational_operator, right_double);
        },
        [relational_operator](const double left_double, const int64_t right_integer) -> RuntimeValue {
          return ComputeRelationalOperation(left_double, relational_operator, right_integer);
        },
        [](const auto&, const auto&) -> RuntimeValue {
          throw std::runtime_error("ExecuteRelationalOperation: unsupported relational operator");
        },
      },
      left_value, right_value
    );
  }

  RuntimeValue
  ExecuteEqualityOperation(const RuntimeValue& left_value, const ast::EqualityOperator equality_operator, const RuntimeValue& right_value) {
    const bool are_equal = std::visit(
      template_helpers::Overloaded{
        [](const int64_t left_integer, const int64_t right_integer) { return left_integer == right_integer; },
        [](const double left_double, const double right_double) { return left_double == right_double; },
        [](const int64_t left_integer, const double right_double) { return left_integer == right_double; },
        [](const double left_double, const int64_t right_integer) { return left_double == right_integer; },
        [](const bool left_bool, const bool right_bool) { return left_bool == right_bool; },
        [](const NothingValue&, const NothingValue&) { return true; },
        [](const auto&, const auto&) { return false; },
      },
      left_value, right_value
    );

    switch (equality_operator) {
      case ast::EqualityOperator::kEqual: return are_equal;
      case ast::EqualityOperator::kNotEqual: return !are_equal;
    }

    throw std::runtime_error("ExecuteEqualityOperation: unsupported equality operator");
  }

  RuntimeValue ExecuteLogicalOperation(const bool left_value, const ast::LogicalOperator logical_operator, const bool right_value) {
    switch (logical_operator) {
      case ast::LogicalOperator::kAnd: return left_value && right_value;
      case ast::LogicalOperator::kOr: return left_value || right_value;
    }

    throw std::runtime_error("ExecuteLogicalOperation: unsupported logical operator");
  }

  RuntimeValue ExecuteUnaryOperation(const ast::UnaryOperator unary_operator, const RuntimeValue& operand_value) {
    switch (unary_operator) {
      case ast::UnaryOperator::kNegate:
        return std::visit(
          template_helpers::Overloaded{
            [](const int64_t integer_value) -> RuntimeValue { return -integer_value; },
            [](const double double_value) -> RuntimeValue { return -double_value; },
            [](const auto&) -> RuntimeValue { throw std::runtime_error("ValidateNumericValue: value must be a double or integer"); },
          },
          operand_value
        );
      case ast::UnaryOperator::kNot: return !IsTruthy(operand_value);
    }

    throw std::runtime_error("ExecuteUnaryOperation: unsupported unary operator");
  }

  void PrintValue(const RuntimeValue& value) {
    std::visit(
      template_helpers::Overloaded{
        [](const int64_t integer_value) { std::cout << integer_value; },
        [](const double double_value) { std::cout << double_value; },
        [](const bool bool_value) { std::cout << (bool_value ? "true" : "false"); },
        [](const NothingValue&) { std::cout << "nothing"; },
      },
      value
    );
  }
} // namespace tree_interpreter

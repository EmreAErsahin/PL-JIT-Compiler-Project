#include <format>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>

#include "../common/overloaded.h"
#include "ast.h"
#include "ast_printer.h"

namespace ast_walk {
  std::string Indentation(const size_t depth) {
    return std::string(depth, '\t');
  }

  std::string ToString(const ast::ExpressionVariant& expression_variant);

  std::string ToString(const std::span<const ast::Identifier> identifiers) {
    std::string joined_identifiers;
    bool first_identifier = true;
    for (const auto& identifier : identifiers) {
      if (!first_identifier) {
        joined_identifiers += ", ";
      }
      joined_identifiers += identifier.name_;
      first_identifier = false;
    }
    return joined_identifiers;
  }

  std::string ToString(std::span<const ast::ExpressionPointer> arguments) {
    std::string joined_arguments;
    bool first_argument = true;
    for (const auto& argument : arguments) {
      if (!first_argument) {
        joined_arguments += ", ";
      }
      joined_arguments += ToString(*argument);
      first_argument = false;
    }
    return joined_arguments;
  }

  constexpr std::string_view ToString(const ast::ArithmeticOperator arithmetic_operator) {
    switch (arithmetic_operator) {
      case ast::ArithmeticOperator::kAdd: return "+";
      case ast::ArithmeticOperator::kSubtract: return "-";
      case ast::ArithmeticOperator::kMultiply: return "*";
      case ast::ArithmeticOperator::kDivide: return "/";
      case ast::ArithmeticOperator::kModulo: return "%";
    }

    throw std::runtime_error("ToString: unsupported arithmetic operator");
  }

  constexpr std::string_view ToString(const ast::RelationalOperator relational_operator) {
    switch (relational_operator) {
      case ast::RelationalOperator::kLessThan: return "<";
      case ast::RelationalOperator::kLessThanOrEqual: return "<=";
      case ast::RelationalOperator::kGreaterThan: return ">";
      case ast::RelationalOperator::kGreaterThanOrEqual: return ">=";
    }

    throw std::runtime_error("ToString: unsupported relational operator");
  }

  constexpr std::string_view ToString(const ast::EqualityOperator equality_operator) {
    switch (equality_operator) {
      case ast::EqualityOperator::kEqual: return "==";
      case ast::EqualityOperator::kNotEqual: return "!=";
    }

    throw std::runtime_error("ToString: unsupported equality operator");
  }

  constexpr std::string_view ToString(const ast::LogicalOperator logical_operator) {
    switch (logical_operator) {
      case ast::LogicalOperator::kAnd: return "&&";
      case ast::LogicalOperator::kOr: return "||";
    }

    throw std::runtime_error("ToString: unsupported logical operator");
  }

  constexpr std::string_view ToString(const ast::UnaryOperator unary_operator) {
    switch (unary_operator) {
      case ast::UnaryOperator::kNegate: return "-";
      case ast::UnaryOperator::kNot: return "!";
    }

    throw std::runtime_error("ToString: unsupported unary operator");
  }

  std::string EscapeStringLiteral(const std::string& string_value) {
    std::string escaped_string;
    for (const char character : string_value) {
      switch (character) {
        case '\n': escaped_string += "\\n"; break;
        case '\t': escaped_string += "\\t"; break;
        case '\\': escaped_string += "\\\\"; break;
        default: escaped_string += character; break;
      }
    }
    return escaped_string;
  }

  std::string ToString(const ast::ExpressionVariant& expression_variant) {
    return std::visit(
      template_helpers::Overloaded{
        [](const ast::IntegerLiteralExpression& integer_expression) -> std::string { return std::to_string(integer_expression.value_); },
        [](const ast::DoubleLiteralExpression& double_expression) -> std::string { return std::format("{:g}", double_expression.value_); },
        [](const ast::BoolLiteralExpression& bool_expression) -> std::string { return bool_expression.value_ ? "true" : "false"; },
        [](const ast::StringLiteralExpression& string_expression) -> std::string {
          return std::format("\"{}\"", EscapeStringLiteral(string_expression.value_));
        },
        [](const ast::ArrayLiteralExpression& array_expression) -> std::string {
          return std::format("[{}]", ToString(array_expression.elements_));
        },
        [](const ast::NothingLiteralExpression&) -> std::string { return "nothing"; },
        [](const ast::IdentifierExpression& identifier_expression) -> std::string { return identifier_expression.identifier_.name_; },
        [](const ast::ArithmeticExpression& arithmetic_expression) -> std::string {
          return std::format(
            "({} {} {})", ToString(*arithmetic_expression.left_operand_), ToString(arithmetic_expression.operator_),
            ToString(*arithmetic_expression.right_operand_)
          );
        },
        [](const ast::RelationalExpression& relational_expression) -> std::string {
          return std::format(
            "({} {} {})", ToString(*relational_expression.left_operand_), ToString(relational_expression.operator_),
            ToString(*relational_expression.right_operand_)
          );
        },
        [](const ast::EqualityExpression& equality_expression) -> std::string {
          return std::format(
            "({} {} {})", ToString(*equality_expression.left_operand_), ToString(equality_expression.operator_),
            ToString(*equality_expression.right_operand_)
          );
        },
        [](const ast::LogicalExpression& logical_expression) -> std::string {
          return std::format(
            "({} {} {})", ToString(*logical_expression.left_operand_), ToString(logical_expression.operator_),
            ToString(*logical_expression.right_operand_)
          );
        },
        [](const ast::UnaryExpression& unary_expression) -> std::string {
          return std::format("({}{})", ToString(unary_expression.operator_), ToString(*unary_expression.operand_));
        },
        [](const ast::FunctionCallExpression& function_call_expression) -> std::string {
          return std::format("{}({})", function_call_expression.function_name_.name_, ToString(function_call_expression.arguments_));
        },
        [](const ast::LengthExpression& length_expression) -> std::string {
          return std::format("len({})", ToString(*length_expression.expression_));
        },
        [](const ast::IndexExpression& index_expression) -> std::string {
          std::string indexed_expression = ToString(*index_expression.indexed_expression_);
          for (const auto& indexing_expression : index_expression.indexing_expressions_) {
            indexed_expression += std::format("[{}]", ToString(*indexing_expression));
          }
          return indexed_expression;
        },
        [](const auto&) -> std::string { throw std::runtime_error("ToString: unsupported expression type"); }
      },
      expression_variant
    );
  }

  std::string ToString(const ast::BlockPointer& block_pointer, const size_t depth) {
    if (!block_pointer) {
      throw std::runtime_error("ToString: null block pointer");
    }

    std::string block_string;
    for (const auto& statement_variant : block_pointer->statements_) {
      std::visit(
        template_helpers::Overloaded{
          [&block_string, depth](const ast::PrintStatement& print_statement) {
            block_string += Indentation(depth) + (print_statement.new_line_ ? "println(" : "print(");
            if (print_statement.print_expression_) {
              block_string += ToString(*print_statement.print_expression_);
            }
            block_string += ");\n";
          },
          [&block_string, depth](const ast::LetStatement& let_statement) {
            block_string += std::format(
              "{}let {} = {};\n", Indentation(depth), let_statement.identifier_.name_, ToString(let_statement.initializer_expression_)
            );
          },
          [&block_string, depth](const ast::AssignmentStatement& assignment_statement) {
            block_string += std::format(
              "{}{} = {};\n", Indentation(depth), assignment_statement.identifier_.name_,
              ToString(assignment_statement.assigned_expression_)
            );
          },
          [&block_string, depth](const ast::IfStatement& if_statement) {
            block_string += std::format("{}if {} {{\n", Indentation(depth), ToString(if_statement.if_condition_));
            block_string += ToString(if_statement.if_block_, depth + 1);
            block_string += Indentation(depth) + "}";

            for (const auto& [else_if_condition, else_if_block] : if_statement.else_if_branches_) {
              block_string += std::format(" else if {} {{\n", ToString(else_if_condition));
              block_string += ToString(else_if_block, depth + 1);
              block_string += Indentation(depth) + "}";
            }

            if (if_statement.else_block_) {
              block_string += " else {\n";
              block_string += ToString(*if_statement.else_block_, depth + 1);
              block_string += Indentation(depth) + "}";
            }

            block_string += "\n";
          },
          [&block_string, depth](const ast::WhileStatement& while_statement) {
            block_string += std::format("{}while {} {{\n", Indentation(depth), ToString(while_statement.while_condition_));
            block_string += ToString(while_statement.while_block_, depth + 1);
            block_string += Indentation(depth) + "}\n";
          },
          [&block_string, depth](const ast::ForStatement& for_statement) {
            block_string += std::format(
              "{}for let {} = {}; {}; {} = {} {{\n", Indentation(depth), for_statement.initializer_.identifier_.name_,
              ToString(for_statement.initializer_.initializer_expression_), ToString(for_statement.condition_),
              for_statement.update_.identifier_.name_, ToString(for_statement.update_.assigned_expression_)
            );
            block_string += ToString(for_statement.for_block_, depth + 1);
            block_string += Indentation(depth) + "}\n";
          },
          [&block_string, depth](const ast::ContinueStatement&) { block_string += Indentation(depth) + "continue;\n"; },
          [&block_string, depth](const ast::BreakStatement&) { block_string += Indentation(depth) + "break;\n"; },
          [&block_string, depth](const ast::ReturnStatement& return_statement) {
            block_string += Indentation(depth) + "return";
            if (return_statement.return_expression_) {
              block_string += " " + ToString(*return_statement.return_expression_);
            }
            block_string += ";\n";
          },
          [&block_string, depth](const ast::BlockPointer& nested_block_pointer) {
            block_string += Indentation(depth) + "{\n";
            block_string += ToString(nested_block_pointer, depth + 1);
            block_string += Indentation(depth) + "}\n";
          },
          [&block_string, depth](const ast::FunctionCallStatement& function_call_statement) {
            block_string +=
              std::format("{}{};\n", Indentation(depth), ToString(ast::ExpressionVariant{function_call_statement.function_call_}));
          },
          [&block_string, depth](const ast::IndexAssignmentStatement& index_assignment_statement) {
            block_string += std::format(
              "{}{} = {};\n", Indentation(depth), ToString(ast::ExpressionVariant{index_assignment_statement.target_}),
              ToString(*index_assignment_statement.assigned_expression_)
            );
          }
        },
        statement_variant
      );
    }
    return block_string;
  }

  std::string ToString(const ast::Program& program) {
    std::string program_string;
    for (const auto& function : program.functions_) {
      program_string += std::format(
        "fn {}({}) {{\n{}}}\n", function.identifier_.name_, ToString(function.parameters_), ToString(function.function_block_, 1)
      );
    }
    return program_string;
  }
} // namespace ast_walk

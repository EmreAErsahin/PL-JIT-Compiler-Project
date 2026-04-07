#include <string>
#include <variant>

#include "../pl_ast.h"
#include "debug_helpers.h"
#include "template_helpers.h"

namespace debug_helpers {
  std::string ToString(const pl_ast::ExpressionVariant& expression_variant) {
    return std::visit(
      template_helpers::Overloaded{
        [](const pl_ast::IntegerLiteralExpression& integer_expression) -> std::string { return std::to_string(integer_expression.value_); },
        [](const pl_ast::BoolLiteralExpression& bool_expression) -> std::string { return bool_expression.value_ ? "true" : "false"; },
        [](const pl_ast::NothingLiteralExpression&) -> std::string { return "nothing"; },
      },
      expression_variant
    );
  }

  // TODO: Far from complete. Needs dynamic return types, parameters, multi-function, etc... Quick solution
  std::string ToString(const pl_ast::Program& program) {
    std::string program_string = "fn " + program.function_.identifier_.name_ + "() {\n";

    for (const auto& statement_variant : program.function_.statements_) {
      std::visit(
        template_helpers::Overloaded{
          [&program_string](const pl_ast::PrintStatement& print_statement) {
            program_string += "\tprint(";
            if (print_statement.expression_) {
              program_string += ToString(*print_statement.expression_);
            }
            program_string += ");\n";
          },
        },
        statement_variant
      );
    }
    program_string += "}\n";
    return program_string;
  }
} // namespace debug_helpers

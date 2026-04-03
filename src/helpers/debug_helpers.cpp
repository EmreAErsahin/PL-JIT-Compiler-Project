#include <string>
#include <variant>

#include "../pl_ast.h"
#include "debug_helpers.h"
#include "template_helpers.h"

namespace debug_helpers {
  // TODO: Far from complete. Needs dynamic return types, parameters, multi-function, etc... Quick solution
  std::string ToString(const pl_ast::Program& program) {
    std::string program_string = "void " + program.function_.name_.value_ + "() {\n";

    for (const auto& instruction_variant : program.function_.instructions_) {
      std::visit(
        template_helpers::Overloaded{
          [&program_string](const pl_ast::PrintInstruction& print_instruction) {
            program_string += "\tprint(";
            if (print_instruction.value_) {
              program_string += std::to_string(print_instruction.value_->value_);
            }
            program_string += ");\n";
          },
        },
        instruction_variant
      );
    }
    program_string += "}\n";
    return program_string;
  }
} // namespace debug_helpers

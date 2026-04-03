#include <iostream>
#include <variant>

#include "helpers/template_helpers.h"
#include "tree_interpreter.h"

namespace tree_interpreter {
  void ExecuteInstruction(const pl_ast::InstructionVariant& instruction_variant) {
    std::visit(
      template_helpers::Overloaded{
        [](const pl_ast::PrintInstruction& print_instruction) {
          if (print_instruction.value_) {
            std::cout << print_instruction.value_->value_;
          }
        },
      },
      instruction_variant
    );
  }

  void ExecuteAstWithTreeInterpreter(const pl_ast::Program& program) {
    for (const auto& instruction_variant : program.function_.instructions_) {
      ExecuteInstruction(instruction_variant);
    }
  }
} // namespace tree_interpreter
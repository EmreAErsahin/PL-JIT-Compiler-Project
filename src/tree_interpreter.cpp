#include <iostream>
#include <stdexcept>
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
    // Enforcing one main function
    const pl_ast::Function* main_function = nullptr;
    // TODO: Make this for once we have multiple functions
    if (program.function_.name_.value_ == "main") {
      main_function = &program.function_;
    }
    if (!main_function) {
      throw std::runtime_error("ExecuteAstWithTreeInterpreter: no main function found");
    }

    for (const auto& instruction_variant : main_function->instructions_) {
      ExecuteInstruction(instruction_variant);
    }
  }
} // namespace tree_interpreter
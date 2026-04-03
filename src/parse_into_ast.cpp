#include "peglib.h"

#include <any>
#include <stdexcept>
#include <string>
#include <vector>

#include "parse_into_ast.h"

namespace parse_into_ast {
  peg::parser MakeParser() {
    peg::parser parser;

    // First rule is start rule for library
    constexpr auto pl_grammar = R"(
      Program      <- Function
      Function     <- Type Identifier '(' ')' '{' Body '}'
      Body         <- Instruction*
      Instruction  <- Print ';'
      Print        <- 'print' '(' Integer? ')'
      Type         <- 'void'
      Identifier   <- < [a-zA-Z_][a-zA-Z0-9_]* >
      Integer      <- < '-'? [0-9]+ >
      %whitespace  <- [ \t\r\n]*
    )";

    if (!parser.load_grammar(pl_grammar)) {
      throw std::runtime_error("MakeParser: failed to load parser grammar");
    }

    parser["Type"] = [](const peg::SemanticValues&) { return pl_ast::Type::VOID; };

    parser["Identifier"] = [](const peg::SemanticValues& semantic_values) { return pl_ast::Identifier{semantic_values.token_to_string()}; };

    parser["Integer"] = [](const peg::SemanticValues& semantic_values) {
      return pl_ast::Integer{semantic_values.token_to_number<int64_t>()};
    };

    parser["Print"] = [](const peg::SemanticValues& semantic_values) {
      pl_ast::PrintInstruction print_instruction;
      if (!semantic_values.empty()) {
        print_instruction.value_ = std::any_cast<pl_ast::Integer>(semantic_values[0]);
      }
      return print_instruction;
    };

    parser["Instruction"] = [](const peg::SemanticValues& semantic_values) {
      return pl_ast::InstructionVariant{std::any_cast<pl_ast::PrintInstruction>(semantic_values[0])};
    };

    parser["Body"] = [](const peg::SemanticValues& semantic_values) {
      std::vector<pl_ast::InstructionVariant> instruction_variants;
      instruction_variants.reserve(semantic_values.size());

      for (const auto& semantic_value : semantic_values) {
        instruction_variants.push_back(std::any_cast<pl_ast::InstructionVariant>(semantic_value));
      }

      return instruction_variants;
    };

    // Using designated initializers for clarity & safety against changing field positions
    parser["Function"] = [](const peg::SemanticValues& semantic_values) {
      return pl_ast::Function{
        .return_type_ = std::any_cast<pl_ast::Type>(semantic_values[0]),
        .name_ = std::any_cast<pl_ast::Identifier>(semantic_values[1]),
        .instructions_ = std::any_cast<std::vector<pl_ast::InstructionVariant>>(semantic_values[2]),
      };
    };

    parser["Program"] = [](const peg::SemanticValues& semantic_values) {
      return pl_ast::Program{.function_ = std::any_cast<pl_ast::Function>(semantic_values[0])};
    };

    // Trades memory usage for better speed with memoization of grammar rules
    parser.enable_packrat_parsing();

    return parser;
  }

  pl_ast::Program ParseFileContentsIntoAST(const std::string& file_contents) {
    auto parser = MakeParser();

    size_t error_line = 0;
    size_t error_column = 0;
    std::string error_message;

    parser.set_logger([&error_line, &error_column, &error_message](const size_t line, const size_t column, const std::string& message) {
      error_line = line;
      error_column = column;
      error_message = message;
    });

    pl_ast::Program program_ast;
    if (!parser.parse(file_contents, program_ast)) {
      // TODO: Sometimes logger won't populate all of these
      throw std::runtime_error(
        "ParseFileContentsIntoAST: parse failed at " + std::to_string(error_line) + ':' + std::to_string(error_column) + ": "
        + error_message
      );
    }

    return program_ast;
  }
} // namespace parse_into_ast

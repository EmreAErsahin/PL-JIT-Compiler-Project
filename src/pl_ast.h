#ifndef pl_ast_H
#define pl_ast_H

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace pl_ast {
  struct Integer {
    int64_t value_;
  };

  struct Identifier {
    std::string value_;
  };

  enum class Type { VOID };

  struct PrintInstruction {
    std::optional<Integer> value_;
  };

  using InstructionVariant = std::variant<PrintInstruction>;

  struct Function {
    Type return_type_;
    Identifier name_;
    std::vector<InstructionVariant> instructions_;
  };

  // TODO: Change to a vector of functions once we allow multiple
  struct Program {
    Function function_;
  };
} // namespace pl_ast

#endif // pl_ast_H
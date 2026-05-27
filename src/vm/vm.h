#ifndef vm_H
#define vm_H

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#include "../ast/ast.h"
#include "../tree_interpreter/tree_interpreter.h"

namespace ee {
  class VM {
   public:
    void LoadFile(const std::filesystem::path& file_path);

    [[nodiscard]] std::string DebugAstString() const;

    void Call(const std::string& function_name);

   private:
    std::unique_ptr<ast::Program> program_ast_;
    std::optional<tree_interpreter::Interpreter> interpreter_;
  };
} // namespace ee

#endif // vm_H
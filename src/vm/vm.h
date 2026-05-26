#ifndef vm_H
#define vm_H

#include <filesystem>
#include <optional>
#include <string>

#include "../ast/ast.h"

namespace ee {
  class VM {
   public:
    void LoadFile(const std::filesystem::path& file_path);

    [[nodiscard]] std::string DebugAstString() const;

    void RunMain() const;

   private:
    std::optional<ast::Program> program_ast_;
  };
} // namespace ee

#endif // vm_H
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "../ast/ast.h"
#include "../ast/ast_printer.h"
#include "../parser/parser.h"
#include "../tree_interpreter/tree_interpreter.h"
#include "vm.h"

namespace ee {
  std::string ReadFile(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
      throw std::runtime_error("ReadFile: failed to open file '" + path.string() + "'");
    }

    std::ostringstream file_contents;
    file_contents << input.rdbuf();
    return file_contents.str();
  }

  void VM::LoadFile(const std::filesystem::path& file_path) {
    const std::string file_contents = ReadFile(file_path); // we define this bc ParseFileContentsIntoAst takes a string_view
    program_ast_ = parser::ParseFileContentsIntoAST(file_contents);
  }

  std::string VM::DebugAstString() const {
    if (!program_ast_) {
      throw std::runtime_error("DebugAstString: no program loaded");
    }
    return ast_walk::ToString(*program_ast_);
  }

  void VM::RunMain() const {
    if (!program_ast_) {
      throw std::runtime_error("RunMain: no program loaded");
    }
    tree_interpreter::ExecuteAstWithTreeInterpreter(*program_ast_);
  }
} // namespace ee
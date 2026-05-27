#include <fstream>
#include <memory>
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
    program_ast_ = std::make_unique<ast::Program>(parser::ParseFileContentsIntoAST(file_contents));
    interpreter_ = tree_interpreter::Interpreter{*program_ast_};
  }

  std::string VM::DebugAstString() const {
    if (!program_ast_) {
      throw std::runtime_error("DebugAstString: no program loaded");
    }
    return ast_walk::ToString(*program_ast_);
  }

  void VM::Call(const std::string& function_name) {
    if (!interpreter_) {
      throw std::runtime_error("VM::Call: load file hasn't been called to establish proper context");
    }
    [[maybe_unused]] tree_interpreter::RuntimeValue return_value = interpreter_->ExecuteFunction(function_name, {});
  }

} // namespace ee
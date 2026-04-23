#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "ast/ast.h"
#include "ast/ast_printer.h"
#include "parser/parser.h"
#include "tree_interpreter/tree_interpreter.h"

std::string ReadFile(const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("ReadFile: failed to open file '" + path.string() + "'");
  }

  std::ostringstream file_contents;
  file_contents << input.rdbuf();
  return file_contents.str();
}

int main(const int argc, char** argv) {
  // Handling options that are passed in by user (--debug)
  bool debug = false;

  std::filesystem::path source_path;

  if (argc == 2) {
    source_path = argv[1];
  } else if (argc == 3) {
    if (const std::string flag = argv[1]; flag != "--debug") {
      std::cerr << "Usage: " << argv[0] << " [--debug] SOURCE\n";
      return 1;
    }
    debug = true;
    source_path = argv[2];
  } else {
    std::cerr << "Usage: " << argv[0] << " [--debug] SOURCE\n";
    return 1;
  }

  // Execution of source code begins here
  try {
    const std::string file_contents = ReadFile(source_path);

    const auto program_ast = parser::ParseFileContentsIntoAST(file_contents);

    if (debug) {
      std::cout << ast_walk::ToString(program_ast);
    }

    tree_interpreter::ExecuteAstWithTreeInterpreter(program_ast);

    return 0;
  }
  // TODO: Eventually want to be able to produce beautiful error message for user
  catch (const std::exception& exception) {
    std::cerr << exception.what() << '\n';
    return 1;
  } catch (...) {
    std::cerr << "Unknown error\n";
    return 1;
  }
}

#include <exception>
#include <iostream>
#include <string>

#include "helpers/debug_helpers.h"
#include "helpers/io_helpers.h"
#include "parse_into_ast.h"
#include "pl_ast.h"
#include "tree_interpreter.h"

int main(const int argc, char** argv) {
  // All options user can pass in
  bool debug = false;

  std::string source_path;

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

  try {
    const std::string file_contents = io_helpers::ReadFile(source_path);

    const auto program_ast = parse_into_ast::ParseFileContentsIntoAST(file_contents);

    if (debug) {
      std::cout << debug_helpers::ToString(program_ast);
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

#include <exception>
#include <iostream>

#include "helpers/debug_helpers.h"
#include "helpers/io_helpers.h"
#include "parse_into_ast.h"
#include "pl_ast.h"
#include "tree_interpreter.h"

int main() {
  try {
    const std::string file_contents = io_helpers::ReadFile("tests/0000_do_nothing.ee");

    const auto program_ast = parse_into_ast::ParseFileContentsIntoAST(file_contents);

    // TODO: Eventually have debug flag for user to pass in where this will print
    std::cout << debug_helpers::ToString(program_ast);

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

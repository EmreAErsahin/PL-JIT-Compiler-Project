#ifndef parser_H
#define parser_H

#include <string>

#include "../ast/ast.h"

namespace parser {
  pl_ast::Program ParseFileContentsIntoAST(const std::string& file_contents);
}

#endif // parser_H

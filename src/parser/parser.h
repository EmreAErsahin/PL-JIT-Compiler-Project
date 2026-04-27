#ifndef parser_H
#define parser_H

#include <string_view>

#include "../ast/ast.h"

namespace parser {
  pl_ast::Program ParseFileContentsIntoAST(std::string_view file_contents);
}

#endif // parser_H

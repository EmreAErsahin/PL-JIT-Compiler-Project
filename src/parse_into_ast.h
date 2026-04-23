#ifndef parse_into_ast_H
#define parse_into_ast_H

#include <string>

#include "ast/ast.h"

namespace parse_into_ast {
  pl_ast::Program ParseFileContentsIntoAST(const std::string& file_contents);
}

#endif // parse_into_ast_H

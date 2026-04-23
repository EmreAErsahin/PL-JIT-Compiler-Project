#ifndef debug_helpers_H
#define debug_helpers_H

#include <string>

#include "ast.h"

namespace ast_walk {
  std::string ToString(const pl_ast::Program& program);
}

#endif // debug_helpers_H
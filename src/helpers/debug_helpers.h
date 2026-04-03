#ifndef debug_helpers_H
#define debug_helpers_H

#include <string>

#include "../pl_ast.h"

namespace debug_helpers {
  std::string ToString(const pl_ast::Program& program);
}

#endif // debug_helpers_H
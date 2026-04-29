#ifndef ast_printer_H
#define ast_printer_H

#include <string>

#include "ast.h"

namespace ast_walk {
  std::string ToString(const ast::Program& program);
}

#endif // ast_printer_H
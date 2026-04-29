#ifndef tree_interpreter_H
#define tree_interpreter_H

#include "../ast/ast.h"

namespace tree_interpreter {
  void ExecuteAstWithTreeInterpreter(const ast::Program& program);
}

#endif // tree_interpreter_H
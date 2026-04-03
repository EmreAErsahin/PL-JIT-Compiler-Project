#ifndef tree_interpreter_H
#define tree_interpreter_H

#include "pl_ast.h"

namespace tree_interpreter {
  void ExecuteAstWithTreeInterpreter(const pl_ast::Program& program);
}

#endif // tree_interpreter_H
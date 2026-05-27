#ifndef runtime_state_H
#define runtime_state_H

#include <string>
#include <unordered_map>
#include <vector>

#include "../ast/ast.h"
#include "runtime_value.h"

namespace tree_interpreter {
  // {Variable Name, runtime value}
  using Scope = std::unordered_map<std::string, RuntimeValue>;

  // A function call frame owns the lexical scopes created while that call is active.
  struct CallFrame {
    std::vector<Scope> scopes_;
  };

  class RuntimeState {
   public:
    struct ScopeGuard {
      explicit ScopeGuard(RuntimeState& runtime_state);

      ~ScopeGuard();

      // Delete copy/move so scope lifetime always matches one guard instance.
      ScopeGuard(const ScopeGuard&) = delete;
      ScopeGuard& operator=(const ScopeGuard&) = delete;
      ScopeGuard(ScopeGuard&&) = delete;
      ScopeGuard& operator=(ScopeGuard&&) = delete;

     private:
      RuntimeState& runtime_state_;
    };

    struct CallFrameGuard {
      explicit CallFrameGuard(RuntimeState& runtime_state);

      ~CallFrameGuard();

      CallFrameGuard(const CallFrameGuard&) = delete;
      CallFrameGuard& operator=(const CallFrameGuard&) = delete;
      CallFrameGuard(CallFrameGuard&&) = delete;
      CallFrameGuard& operator=(CallFrameGuard&&) = delete;

     private:
      RuntimeState& runtime_state_;
    };

    void BuildFunctionTable(const ast::Program& program);

    const ast::Function& LookupFunctionOrThrow(const std::string& function_name) const;

    const RuntimeValue& LookupVariable(const std::string& variable_name) const;

    void DeclareVariable(const std::string& variable_name, RuntimeValue value);

    void AssignVariable(const std::string& variable_name, RuntimeValue value);

   private:
    std::vector<CallFrame> call_stack_;
    std::unordered_map<std::string, const ast::Function*> available_functions_;

    CallFrame& CurrentFrame();

    Scope& CurrentScope();
  };
} // namespace tree_interpreter

#endif // runtime_state_H

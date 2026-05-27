#include <ranges>
#include <stdexcept>

#include "runtime_state.h"

namespace tree_interpreter {
  // Using RAII to add/delete scopes on construction / destruction.
  RuntimeState::ScopeGuard::ScopeGuard(RuntimeState& runtime_state) : runtime_state_(runtime_state) {
    runtime_state_.CurrentFrame().scopes_.emplace_back();
  }

  RuntimeState::ScopeGuard::~ScopeGuard() {
    runtime_state_.CurrentFrame().scopes_.pop_back();
  }

  RuntimeState::CallFrameGuard::CallFrameGuard(RuntimeState& runtime_state) : runtime_state_(runtime_state) {
    runtime_state_.call_stack_.emplace_back();
  }

  RuntimeState::CallFrameGuard::~CallFrameGuard() {
    runtime_state_.call_stack_.pop_back();
  }

  void RuntimeState::BuildFunctionTable(const ast::Program& program) {
    // Function declarations are global and callable regardless of source order.
    for (const auto& current_function : program.functions_) {
      if (available_functions_.contains(current_function.identifier_.name_)) {
        throw std::runtime_error("BuildFunctionTable: duplicate function '" + current_function.identifier_.name_ + "'");
      }
      available_functions_.emplace(current_function.identifier_.name_, &current_function);
    }
  }

  const ast::Function& RuntimeState::LookupFunctionOrThrow(const std::string& function_name) const {
    if (const auto function_iterator = available_functions_.find(function_name); function_iterator != available_functions_.end()) {
      return *function_iterator->second;
    }

    throw std::runtime_error("EvaluateExpression: unknown function '" + function_name + "'");
  }

  const RuntimeValue& RuntimeState::LookupVariable(const std::string& variable_name) const {
    // Lexical lookup starts in the innermost scope and walks outward within the current call frame.
    for (const auto& scope : std::views::reverse(call_stack_.back().scopes_)) {
      if (const auto variable_iterator = scope.find(variable_name); variable_iterator != scope.end()) {
        return variable_iterator->second;
      }
    }

    throw std::runtime_error("EvaluateExpression: unknown variable '" + variable_name + "'");
  }

  void RuntimeState::DeclareVariable(const std::string& variable_name, RuntimeValue value) {
    auto& current_scope = CurrentScope();
    if (const auto variable_iterator = current_scope.find(variable_name); variable_iterator != current_scope.end()) {
      throw std::runtime_error("ExecuteStatement: variable '" + variable_name + "' already declared");
    }

    current_scope.emplace(variable_name, value);
  }

  void RuntimeState::AssignVariable(const std::string& variable_name, RuntimeValue value) {
    // Assignment updates the nearest visible binding, matching lexical shadowing rules.
    for (auto& scope : std::views::reverse(CurrentFrame().scopes_)) {
      if (auto variable_iterator = scope.find(variable_name); variable_iterator != scope.end()) {
        variable_iterator->second = value;
        return;
      }
    }

    throw std::runtime_error("ExecuteStatement: variable '" + variable_name + "' is not declared");
  }

  CallFrame& RuntimeState::CurrentFrame() {
    return call_stack_.back();
  }

  Scope& RuntimeState::CurrentScope() {
    return CurrentFrame().scopes_.back();
  }
} // namespace tree_interpreter

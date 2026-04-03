#ifndef template_helpers_H
#define template_helpers_H

#include <utility>

namespace template_helpers {
  template <class... Ts>
  struct Overloaded : Ts... {
    using Ts::operator()...;
  };

  template <class... Ts>
  Overloaded(Ts...) -> Overloaded<Ts...>;
} // namespace template_helpers

#endif // template_helpers_H

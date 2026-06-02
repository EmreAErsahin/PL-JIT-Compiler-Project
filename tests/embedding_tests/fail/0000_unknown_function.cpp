#include <exception>
#include <iostream>

#include "vm/vm.h"

int main() {
  try {
    ee::VM vm;

    vm.LoadFile("0000_unknown_function.ee");

    vm.Call("missing");
  } catch (const std::exception& exception) {
    std::cerr << exception.what() << '\n';

    return 1;
  }
}

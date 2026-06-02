#include "vm/vm.h"

int main() {
  ee::VM vm;

  vm.LoadFile("0000_hello_world.ee");

  vm.Call("main");

  return 0;
}
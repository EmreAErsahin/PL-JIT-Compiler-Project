#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

#include "vm/vm.h"

int main(const int argc, char** argv) {
  bool debug = false;

  std::filesystem::path source_path;

  if (argc == 2) {
    source_path = argv[1];
  } else if (argc == 3) {
    if (const std::string flag = argv[1]; flag != "--debug") {
      std::cerr << "Usage: " << argv[0] << " [--debug] SOURCE\n";
      return 1;
    }
    debug = true;
    source_path = argv[2];
  } else {
    std::cerr << "Usage: " << argv[0] << " [--debug] SOURCE\n";
    return 1;
  }

  try {
    ee::VM vm;

    vm.LoadFile(source_path);

    if (debug) {
      std::cout << vm.DebugAstString();
    }

    vm.RunMain();

    return 0;
  }
  // TODO: Eventually want to be able to produce beautiful error message for user
  catch (const std::exception& exception) {
    std::cerr << exception.what() << '\n';
    return 1;
  } catch (...) {
    std::cerr << "Unknown error\n";
    return 1;
  }
}

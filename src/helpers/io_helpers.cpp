#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "io_helpers.h"

namespace io_helpers {
  std::string ReadFile(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
      throw std::runtime_error("ReadFile: failed to open file '" + path.string() + "'");
    }

    std::ostringstream file_contents;
    file_contents << input.rdbuf();
    return file_contents.str();
  }
} // namespace io_helpers

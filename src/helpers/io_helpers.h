#ifndef io_helpers_H
#define io_helpers_H

#include <filesystem>
#include <string>

namespace io_helpers {
  std::string ReadFile(const std::filesystem::path& path);
}

#endif // io_helpers_H
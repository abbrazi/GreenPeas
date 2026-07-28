#ifndef GREENPEAS_CLI_IO_HPP
#define GREENPEAS_CLI_IO_HPP

/// Standard headers
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

/// Project headers
#include "GreenPeas/Common.hpp"

#ifndef GP_DATA_PATH
#define GP_DATA_PATH "data"
#endif

namespace gp {

/// @brief Write a prefixed log line to stdout.
/// @param msg Message body.
HOST inline void log(std::string_view msg) {
  std::cout << "[GreenPeas AE] " << msg << "\n";
}

/// @brief Absolute path to a results CSV under `data/ae/`.
/// @param filename Result CSV filename.
/// @return Path to the CSV file.
HOST inline auto csvPath(std::string_view filename) -> std::filesystem::path {
  return std::filesystem::path(GP_DATA_PATH) / "ae" / std::string(filename);
}

/// @brief Write a CSV header under `data/ae/`.
/// @param filename Result CSV filename.
/// @param header Comma-separated header row (no trailing newline).
HOST inline void writeResultsCsvHeader(std::string_view filename,
                                       std::string_view header) {
  const auto file = csvPath(filename);
  std::filesystem::create_directories(file.parent_path());
  std::ofstream out(file, std::ios::trunc);
  if (!out) {
    throw std::runtime_error("Failed to write results CSV: " + file.string());
  }
  out << header << '\n';
}

/// @brief Append a CSV data row under `data/ae/`.
/// @param filename Result CSV filename.
/// @param row Comma-separated data row (no trailing newline).
HOST inline void appendResultsCsvRow(std::string_view filename,
                                     std::string_view row) {
  const auto file = csvPath(filename);
  std::ofstream out(file, std::ios::app);
  if (!out) {
    throw std::runtime_error("Failed to append results CSV: " + file.string());
  }
  out << row << '\n';
}

} // namespace gp

#endif // GREENPEAS_CLI_IO_HPP

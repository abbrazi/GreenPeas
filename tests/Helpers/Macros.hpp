#ifndef GREENPEAS_TESTS_HELPERS_MACROS_HPP
#define GREENPEAS_TESTS_HELPERS_MACROS_HPP

/// Standard headers
#include <cmath>
#include <cstdlib>
#include <iostream>

[[noreturn]] inline void
handle_failure(const char *file, int line, std::string_view condition) {
  std::cerr << "--- TEST FAILURE ---\n"
            << "File:      " << file << "\n"
            << "Line:      " << line << "\n"
            << "Condition: " << condition << "\n";
  std::quick_exit(EXIT_FAILURE);
}

#define REQUIRE(condition)                                                     \
  if (!(condition)) {                                                          \
    handle_failure(__FILE__, __LINE__, #condition);                            \
  }

#define REQUIRE_NEAR(actual, expected)                                         \
  if (std::abs((actual) - (expected)) > 1e-15) {                               \
    handle_failure(__FILE__,                                                   \
                   __LINE__,                                                   \
                   "REQUIRE_NEAR: |" #actual " - " #expected "| > 1e-15");     \
  }

#endif // GREENPEAS_TESTS_HELPERS_MACROS_HPP

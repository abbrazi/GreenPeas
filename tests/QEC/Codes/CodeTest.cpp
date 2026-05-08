/// Standard headers
#include <cstdio>
#include <stdexcept>
#include <string>

/// Helper headers
#include "../../Helpers/Macros.hpp"

/// Project headers
#include "GreenPeas/QEC/Codes/Code.hpp"

using namespace gp;

struct TestCase {
  CodeType type;
  uint32_t d;
  std::string circuitPath;
};

static auto parseCircuit(const std::string &path) -> stim::Circuit {
  FILE *file = fopen(path.c_str(), "r");
  if (file == nullptr) {
    throw std::runtime_error("Failed to open circuit file: " + path);
  }
  const stim::Circuit circuit = stim::Circuit::from_file(file);
  fclose(file);
  return circuit;
}

static void testCodeGetMemory(const TestCase &testCase) {
  const Code code(testCase.type, testCase.d, GP_DATA_PATH);
  const stim::Circuit actual = code.getMemory(testCase.d, 0.001);
  const stim::Circuit expected = parseCircuit(testCase.circuitPath);

  REQUIRE(actual == expected);
}

auto main() -> int {
  const std::string root = GP_DATA_PATH;
  const TestCase cases[] = {
      {CodeType::Surface, 3, root + "/circuits/surface3.stim"},
      {CodeType::Surface, 5, root + "/circuits/surface5.stim"},
      {CodeType::Surface, 7, root + "/circuits/surface7.stim"},
      {CodeType::Surface, 9, root + "/circuits/surface9.stim"},
      {CodeType::BB, 6, root + "/circuits/bb6.stim"},
      {CodeType::BB, 10, root + "/circuits/bb10.stim"},
      {CodeType::BB, 12, root + "/circuits/bb12.stim"},
  };

  for (const auto &testCase : cases) {
    testCodeGetMemory(testCase);
  }

  return 0;
}

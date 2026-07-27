/// Standard headers
#include <cstdio>
#include <stdexcept>
#include <string>

/// Helper headers
#include "../../Helpers/Macros.hpp"

/// Project headers
#include "GreenPeas/QEC/Codes/ConcatenatedSurfaceCode.hpp"

using namespace gp;

struct TestCase {
  uint32_t d;
  MeasurementStrategy strategy;
  double p;
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

static void testConcatenatedSurfaceCodeGetMemory(const TestCase &testCase) {
  ConcatenatedSurfaceCode code(testCase.d, GP_DATA_PATH);
  const auto [actual, shot] =
      code.getMemory(testCase.d, testCase.p, testCase.strategy);
  (void)shot;
  const stim::Circuit expected = parseCircuit(testCase.circuitPath);

  REQUIRE(actual == expected);
}

auto main() -> int {
  const std::string root = GP_DATA_PATH;
  const TestCase cases[] = {
      {4,
       MeasurementStrategy::Static,
       0.001,
       root + "/circuits/iceberg_surface_4_static.stim"},
      {6,
       MeasurementStrategy::Static,
       0.001,
       root + "/circuits/iceberg_surface_6_static.stim"},
      {4,
       MeasurementStrategy::Adaptive,
       0.0,
       root + "/circuits/iceberg_surface_4_adaptive.stim"},
      {6,
       MeasurementStrategy::Adaptive,
       0.0,
       root + "/circuits/iceberg_surface_6_adaptive.stim"},
  };

  for (const auto &testCase : cases) {
    testConcatenatedSurfaceCodeGetMemory(testCase);
  }

  return 0;
}

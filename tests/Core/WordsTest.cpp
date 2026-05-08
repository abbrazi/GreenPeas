/// Standard headers
#include <cstdint>

/// Helper headers
#include "../Helpers/Macros.hpp"

/// Project headers
#include "GreenPeas/Core/Words.hpp"

using namespace gp;

// --- Words ---

static void testWordsGetLower() {
  constexpr uint64_t word = (uint64_t{0xFFFFFFFF} << 32) | uint64_t{0xDEADBEEF};
  REQUIRE(getLower(word) == 0xDEADBEEF);
}

static void testWordsGetUpper() {
  constexpr uint64_t word = (uint64_t{0xDEADBEEF} << 32) | uint64_t{0xFFFFFFFF};
  REQUIRE(getUpper(word) == 0xDEADBEEF);
}

static void testWordsSetLower() {
  uint64_t word = (uint64_t{0xFFFFFFFF} << 32) | uint64_t{0x11111111};
  setLower(word, 0xDEADBEEF);
  REQUIRE(getLower(word) == 0xDEADBEEF);
  REQUIRE(getUpper(word) == 0xFFFFFFFF);
}

static void testWordsSetUpper() {
  uint64_t word = (uint64_t{0x11111111} << 32) | uint64_t{0xFFFFFFFF};
  setUpper(word, 0xDEADBEEF);
  REQUIRE(getUpper(word) == 0xDEADBEEF);
  REQUIRE(getLower(word) == 0xFFFFFFFF);
}

static void testWordsNumWordsForBits() {
  REQUIRE(numWordsForBits<uint64_t>(32) == 1);
  REQUIRE(numWordsForBits<uint64_t>(64) == 1);
  REQUIRE(numWordsForBits<uint64_t>(65) == 2);
}

auto main() -> int {
  // --- Words ---

  // getLower
  testWordsGetLower();

  // getUpper
  testWordsGetUpper();

  // setLower
  testWordsSetLower();

  // setUpper
  testWordsSetUpper();

  // numWordsForBits
  testWordsNumWordsForBits();

  // All tests passed!
  return 0;
}

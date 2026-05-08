/// Standard headers
#include <cstdint>

/// Helper headers
#include "../Helpers/Macros.hpp"

/// Project headers
#include "GreenPeas/Core/Array.hpp"

using namespace gp;

using Array4 = Array<uint32_t, 4>;

// --- Array ---

static void testArrayGetElement() {
  Array4 array{};
  array.data[0] = 1;
  array.data[1] = 2;
  array.data[2] = 3;
  array.data[3] = 4;

  REQUIRE(array[0] == 1);
  REQUIRE(array[1] == 2);
  REQUIRE(array[2] == 3);
  REQUIRE(array[3] == 4);
}

static void testArraySetElement() {
  Array4 array{};
  array[0] = 1;
  array[1] = 2;
  array[2] = 3;
  array[3] = 4;

  REQUIRE(array.data[0] == 1);
  REQUIRE(array.data[1] == 2);
  REQUIRE(array.data[2] == 3);
  REQUIRE(array.data[3] == 4);
}

static void testArrayConstElementAccess() {
  Array4 array{};
  array[1] = 99;
  const Array4 &constArray = array;
  REQUIRE(constArray[1] == 99);
}

static void testArrayHash32EqualForEqualContents() {
  Array4 array1{};
  Array4 array2{};
  array1[0] = 1;
  array1[1] = 2;
  array1[2] = 3;
  array1[3] = 4;
  array2[0] = 1;
  array2[1] = 2;
  array2[2] = 3;
  array2[3] = 4;

  REQUIRE(array1.hash32() == array2.hash32());
}

static void testArrayHash32DiffersWhenContentsDiffer() {
  Array4 array1{};
  Array4 array2{};
  array1[0] = 1;
  array1[1] = 2;
  array1[2] = 3;
  array1[3] = 4;
  array2[0] = 1;
  array2[1] = 2;
  array2[2] = 3;
  array2[3] = 5;

  REQUIRE(array1.hash32() != array2.hash32());
}

static void testArrayHash64EqualForEqualContents() {
  Array4 array1{};
  Array4 array2{};
  array1[0] = 1;
  array1[1] = 2;
  array1[2] = 3;
  array1[3] = 4;
  array2[0] = 1;
  array2[1] = 2;
  array2[2] = 3;
  array2[3] = 4;

  REQUIRE(array1.hash64() == array2.hash64());
}

static void testArrayHash64DiffersWhenContentsDiffer() {
  Array4 array1{};
  Array4 array2{};
  array1[0] = 1;
  array1[1] = 2;
  array1[2] = 3;
  array1[3] = 4;
  array2[0] = 1;
  array2[1] = 2;
  array2[2] = 3;
  array2[3] = 5;

  REQUIRE(array1.hash64() != array2.hash64());
}

auto main() -> int {
  // --- Array ---

  // Array::operator[] (non-const)
  testArrayGetElement();
  testArraySetElement();

  // Array::operator[] (const)
  testArrayConstElementAccess();

  // Array::hash32
  testArrayHash32EqualForEqualContents();
  testArrayHash32DiffersWhenContentsDiffer();

  // Array::hash64
  testArrayHash64EqualForEqualContents();
  testArrayHash64DiffersWhenContentsDiffer();

  // All tests passed!
  return 0;
}

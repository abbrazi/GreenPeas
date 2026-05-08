/// Standard headers
#include <cstdint>
#include <stdexcept>
#include <utility>

/// Helper headers
#include "../Helpers/CountingStorage.hpp"
#include "../Helpers/Macros.hpp"

/// Project headers
#include "GreenPeas/Core/Vector.hpp"
#include "GreenPeas/Policies/Storage/Host.hpp"

using namespace gp;

using HostVector = Vector<uint32_t, uint64_t, HostStorage>;

using CountingVector = Vector<uint32_t, uint64_t, CountingStorage>;

// --- VectorView ---

static void testVectorViewDefaultConstructor() {
  VectorView<uint32_t, uint64_t> view;
  REQUIRE(view.size == 0);
  REQUIRE(view.data == nullptr);
}

// --- Vector ---

static void testVectorExplicitConstructor() {
  constexpr uint32_t n = 20;
  HostVector vector(n);
  REQUIRE(vector.size == n);
  REQUIRE(vector.maxSize == n);
  REQUIRE(vector.data != nullptr);
}

static void testVectorDestructor() {
  resetCountingStorage();

  {
    CountingVector vector(3);
    (void)vector;
    REQUIRE(CountingStorage::allocations == 1);
    REQUIRE(CountingStorage::deallocations == 0);
  }

  REQUIRE(CountingStorage::deallocations == 1);
}

static void testVectorCopyConstructor() {
  resetCountingStorage();

  CountingVector src(3);
  src[0] = 7;
  src[2] = 42;

  CountingVector dst(src);

  REQUIRE(dst.size == src.size);
  REQUIRE(dst.maxSize == src.size);
  REQUIRE(dst.data != src.data);
  REQUIRE(dst[0] == 7);
  REQUIRE(dst[2] == 42);

  src[0] = 9;
  REQUIRE(dst[0] == 7);
}

static void testVectorCopyAssignment() {
  resetCountingStorage();

  CountingVector src(3);
  src[1] = 11;

  CountingVector dst(5);

  auto deallocationsBefore = CountingStorage::deallocations;
  auto allocationsBefore = CountingStorage::allocations;

  dst = src;

  REQUIRE(dst.size == src.size);
  REQUIRE(dst.maxSize == src.size);
  REQUIRE(dst.data != src.data);
  REQUIRE(dst[1] == 11);
  REQUIRE(CountingStorage::deallocations == deallocationsBefore + 1);
  REQUIRE(CountingStorage::allocations == allocationsBefore + 1);

  // Self-assignment should be a no-op.
  // NOLINTNEXTLINE(misc-redundant-expression)
  dst = dst;
  REQUIRE(dst[1] == 11);
}

static void testVectorMoveConstructor() {
  resetCountingStorage();

  CountingVector src(3);
  src[1] = 13;

  const auto srcData = src.data;
  const auto srcSize = src.size;
  const auto srcMaxSize = src.maxSize;

  CountingVector dst(std::move(src));

  REQUIRE(dst.data == srcData);
  REQUIRE(dst.size == srcSize);
  REQUIRE(dst.maxSize == srcMaxSize);
  REQUIRE(dst[1] == 13);
  REQUIRE(src.data == nullptr);
  REQUIRE(src.size == 0);
  REQUIRE(src.maxSize == 0);
}

static void testVectorMoveAssignment() {
  resetCountingStorage();

  CountingVector src(3);
  src[0] = 17;
  const auto *srcData = src.data;
  const uint32_t srcSize = src.size;

  CountingVector dst(5);
  auto deallocationsBefore = CountingStorage::deallocations;

  dst = std::move(src);

  REQUIRE(dst.data == srcData);
  REQUIRE(dst.size == srcSize);
  REQUIRE(dst[0] == 17);
  REQUIRE(src.data == nullptr);
  REQUIRE(src.size == 0);
  REQUIRE(src.maxSize == 0);
  REQUIRE(CountingStorage::deallocations == deallocationsBefore + 1);
}

static void testVectorFittoOk() {
  HostVector vector(4);
  REQUIRE(vector.size == 4);
  REQUIRE(vector.maxSize == 4);
  vector.fitto(2);
  REQUIRE(vector.size == 2);
  REQUIRE(vector.maxSize == 4);
}

static void testVectorFittoThrowsWhenExceedsMax() {
  HostVector vector(4);
  bool threw = false;
  try {
    vector.fitto(5);
  } catch (const std::runtime_error &) {
    threw = true;
  }
  REQUIRE(threw);
  REQUIRE(vector.size == 4);
}

static void testVectorGetElement() {
  HostVector vector(5);
  constexpr uint32_t index = 2;
  vector.data[index] = 10;
  REQUIRE(vector[index] == 10);
}

static void testVectorSetElement() {
  HostVector vector(5);
  constexpr uint32_t index = 2;
  vector[index] = 10;
  REQUIRE(vector.data[index] == 10);
}

static void testVectorConstElementAccess() {
  HostVector vector(3);
  vector[1] = 99;
  const HostVector &constVector = vector;
  REQUIRE(constVector[1] == 99);
}

static void testVectorCopyToHost() {
  HostVector vector1(3);
  vector1[0] = 1;
  vector1[2] = 2;
  HostVector vector2(3);
  vector1.copyTo(vector2);
  REQUIRE(vector2[0] == 1);
  REQUIRE(vector2[2] == 2);
}

static void testVectorCopyToThrowsOnSizeMismatch() {
  HostVector vector1(2);
  HostVector vector2(3);
  bool threw = false;
  try {
    vector1.copyTo(vector2);
  } catch (const std::runtime_error &) {
    threw = true;
  }
  REQUIRE(threw);
}

static void testVectorCopyFromHost() {
  HostVector vector1(3);
  vector1[0] = 1;
  vector1[2] = 2;
  HostVector vector2(3);
  vector2.copyFrom(vector1);
  REQUIRE(vector2[0] == 1);
  REQUIRE(vector2[2] == 2);
}

static void testVectorCopyFromThrowsOnSizeMismatch() {
  HostVector vector1(2);
  HostVector vector2(3);
  bool threw = false;
  try {
    vector1.copyFrom(vector2);
  } catch (const std::runtime_error &) {
    threw = true;
  }
  REQUIRE(threw);
}

static void testVectorSet() {
  HostVector vector(4);
  vector.set();
  REQUIRE(vector[0] == UINT64_MAX);
  REQUIRE(vector[1] == UINT64_MAX);
  REQUIRE(vector[2] == UINT64_MAX);
  REQUIRE(vector[3] == UINT64_MAX);
}

static void testVectorClear() {
  HostVector vector(4);
  vector[0] = 1;
  vector[1] = 2;
  vector[2] = 3;
  vector[3] = 4;
  vector.clear();
  REQUIRE(vector[0] == 0);
  REQUIRE(vector[1] == 0);
  REQUIRE(vector[2] == 0);
  REQUIRE(vector[3] == 0);
}

static void testVectorGetView() {
  HostVector vector(2);
  vector[0] = 42;
  auto view = vector.getView();
  REQUIRE(view.size == vector.size);
  REQUIRE(view.data == vector.data);
  // VectorView::operator[] (const)
  REQUIRE(std::as_const(view)[0] == 42);
  // VectorView::operator[] (non-const)
  view[1] = 100;
  REQUIRE(vector[1] == 100);
  REQUIRE(std::as_const(view)[1] == 100);
}

auto main() -> int {
  // --- VectorView ---

  // Constructor
  testVectorViewDefaultConstructor();

  // --- Vector ---

  // Constructor
  testVectorExplicitConstructor();

  // Rule of five
  testVectorDestructor();
  testVectorCopyConstructor();
  testVectorCopyAssignment();
  testVectorMoveConstructor();
  testVectorMoveAssignment();

  // Vector::fitto
  testVectorFittoOk();
  testVectorFittoThrowsWhenExceedsMax();

  // Vector::operator[] (non-const)
  testVectorGetElement();
  testVectorSetElement();

  // Vector::operator[] (const)
  testVectorConstElementAccess();

  // Vector::copyTo
  testVectorCopyToHost();
  testVectorCopyToThrowsOnSizeMismatch();

  // Vector::copyFrom
  testVectorCopyFromHost();
  testVectorCopyFromThrowsOnSizeMismatch();

  // Vector::set
  testVectorSet();

  // Vector::clear
  testVectorClear();

  // Vector::getView
  testVectorGetView();

  // All tests passed!
  return 0;
}

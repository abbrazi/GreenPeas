/// Standard headers
#include <cstdint>
#include <utility>

/// Helper headers
#include "../Helpers/CountingStorage.hpp"
#include "../Helpers/Macros.hpp"

/// Project headers
#include "GreenPeas/Core/Scalar.hpp"
#include "GreenPeas/Policies/Storage/Host.hpp"

using namespace gp;

using HostScalar = Scalar<uint32_t, HostStorage>;

using CountingScalar = Scalar<uint32_t, CountingStorage>;

// --- Scalar ---

static void testScalarDefaultConstructor() {
  HostScalar scalar;
  REQUIRE(scalar.data != nullptr);
}

static void testScalarDestructor() {
  resetCountingStorage();

  {
    CountingScalar scalar;
    (void)scalar;
    REQUIRE(CountingStorage::allocations == 1);
    REQUIRE(CountingStorage::deallocations == 0);
  }

  REQUIRE(CountingStorage::deallocations == 1);
}

static void testScalarCopyConstructor() {
  resetCountingStorage();

  CountingScalar src;
  *src = 7;

  CountingScalar dst(src);

  REQUIRE(dst.data != src.data);
  REQUIRE(*dst == 7);

  *src = 9;
  REQUIRE(*dst == 7);
}

static void testScalarCopyAssignment() {
  resetCountingStorage();

  CountingScalar src;
  *src = 11;

  CountingScalar dst;
  *dst = 0;

  auto deallocationsBefore = CountingStorage::deallocations;
  auto allocationsBefore = CountingStorage::allocations;

  dst = src;

  REQUIRE(dst.data != src.data);
  REQUIRE(*dst == 11);
  REQUIRE(CountingStorage::deallocations == deallocationsBefore + 1);
  REQUIRE(CountingStorage::allocations == allocationsBefore + 1);

  // Self-assignment should be a no-op.
  // NOLINTNEXTLINE(misc-redundant-expression)
  dst = dst;
  REQUIRE(*dst == 11);
}

static void testScalarMoveConstructor() {
  resetCountingStorage();

  CountingScalar src;
  *src = 13;

  const auto *srcData = src.data;

  CountingScalar dst(std::move(src));

  REQUIRE(dst.data == srcData);
  REQUIRE(*dst == 13);
  REQUIRE(src.data == nullptr);
}

static void testScalarMoveAssignment() {
  resetCountingStorage();

  CountingScalar src;
  *src = 17;
  const auto *srcData = src.data;

  CountingScalar dst;
  auto deallocationsBefore = CountingStorage::deallocations;

  dst = std::move(src);

  REQUIRE(dst.data == srcData);
  REQUIRE(*dst == 17);
  REQUIRE(src.data == nullptr);
  REQUIRE(CountingStorage::deallocations == deallocationsBefore + 1);
}

static void testScalarDereferenceNonConst() {
  HostScalar scalar;
  *scalar = 5;
  REQUIRE(*scalar == 5);
}

static void testScalarDereferenceConst() {
  HostScalar scalar;
  *scalar = 99;
  const HostScalar &constScalar = scalar;
  REQUIRE(*constScalar == 99);
}

static void testScalarCopyToHost() {
  HostScalar src;
  *src = 1;
  HostScalar dst;
  *dst = 9;
  src.copyTo(dst);
  REQUIRE(*dst == 1);
}

static void testScalarCopyFromHost() {
  HostScalar src;
  *src = 1;
  HostScalar dst;
  *dst = 0;
  dst.copyFrom(src);
  REQUIRE(*dst == 1);
}

static void testScalarCopyToValue() {
  HostScalar scalar;
  *scalar = 55;
  uint32_t out = 0;
  scalar.copyTo(out);
  REQUIRE(out == 55);
}

static void testScalarCopyFromValue() {
  HostScalar scalar;
  const uint32_t in = 66;
  scalar.copyFrom(in);
  REQUIRE(*scalar == 66);
}

static void testScalarSet() {
  HostScalar scalar;
  scalar.set();
  REQUIRE(*scalar == UINT32_MAX);
}

static void testScalarClear() {
  HostScalar scalar;
  *scalar = 1;
  scalar.clear();
  REQUIRE(*scalar == 0);
}

static void testScalarGetView() {
  HostScalar scalar;
  *scalar = 42;
  auto view = scalar.getView();
  REQUIRE(view.data == scalar.data);
  // ScalarView::operator* (const)
  REQUIRE(*std::as_const(view) == 42);
  // ScalarView::operator* (non-const)
  *view = 100;
  REQUIRE(*scalar == 100);
  REQUIRE(*std::as_const(view) == 100);
}

auto main() -> int {
  // --- Scalar ---

  // Constructor
  testScalarDefaultConstructor();

  // Rule of five
  testScalarDestructor();
  testScalarCopyConstructor();
  testScalarCopyAssignment();
  testScalarMoveConstructor();
  testScalarMoveAssignment();

  // Scalar::operator* (non-const)
  testScalarDereferenceNonConst();

  // Scalar::operator* (const)
  testScalarDereferenceConst();

  // Scalar::copyTo
  testScalarCopyToHost();
  testScalarCopyToValue();

  // Scalar::copyFrom
  testScalarCopyFromHost();
  testScalarCopyFromValue();

  // Scalar::set
  testScalarSet();

  // Scalar::clear
  testScalarClear();

  // Scalar::getView
  testScalarGetView();

  // All tests passed!
  return 0;
}

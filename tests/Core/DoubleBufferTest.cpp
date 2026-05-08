/// Standard headers
#include <utility>

/// Helper headers
#include "../Helpers/Macros.hpp"

/// Project headers
#include "GreenPeas/Core/DoubleBuffer.hpp"
#include "GreenPeas/Core/Matrix.hpp"
#include "GreenPeas/Core/Vector.hpp"
#include "GreenPeas/Policies/Data/Layout.hpp"
#include "GreenPeas/Policies/Storage/Host.hpp"

using namespace gp;

template <typename Layout>
using HostMatrix = Matrix<uint32_t, uint64_t, HostStorage, Layout>;

using HostVector = Vector<uint32_t, uint64_t, HostStorage>;

template <typename Layout>
static void testDoubleBufferMatrix() {
  constexpr uint32_t numRows = 2;
  constexpr uint32_t numCols = 3;
  DoubleBuffer<HostMatrix<Layout>> buffer(numRows, numCols);
  buffer.a(0, 0) = 1;
  buffer.b(1, 2) = 9;
  REQUIRE(buffer.a(0, 0) == 1);
  REQUIRE(buffer.b(1, 2) == 9);
}

template <typename Layout>
static void testDoubleBufferMatrixGetView() {
  constexpr uint32_t numRows = 2;
  constexpr uint32_t numCols = 3;
  DoubleBuffer<HostMatrix<Layout>> buffer(numRows, numCols);
  buffer.a(0, 0) = 1;
  buffer.b(1, 2) = 9;
  auto view = buffer.getView();
  REQUIRE(view.a.dimensions.numRows == numRows);
  REQUIRE(view.a.dimensions.numCols == numCols);
  REQUIRE(view.a.data == buffer.a.data);
  REQUIRE(view.b.data == buffer.b.data);
  REQUIRE(std::as_const(view).a(0, 0) == 1);
  REQUIRE(std::as_const(view).b(1, 2) == 9);
  view.a(0, 1) = 7;
  REQUIRE(buffer.a(0, 1) == 7);
}

static void testDoubleBufferVector() {
  constexpr uint32_t n = 4;
  DoubleBuffer<HostVector> buffer(n);
  buffer.a[0] = 10;
  buffer.b[0] = 20;
  REQUIRE(buffer.a[0] == 10);
  REQUIRE(buffer.b[0] == 20);
}

static void testDoubleBufferVectorGetView() {
  constexpr uint32_t n = 4;
  DoubleBuffer<HostVector> buffer(n);
  buffer.a[0] = 10;
  buffer.b[1] = 20;
  auto view = buffer.getView();
  REQUIRE(view.a.size == buffer.a.size);
  REQUIRE(view.a.data == buffer.a.data);
  REQUIRE(view.b.data == buffer.b.data);
  REQUIRE(std::as_const(view).a[0] == 10);
  REQUIRE(std::as_const(view).b[1] == 20);
  view.a[2] = 30;
  REQUIRE(buffer.a[2] == 30);
}

auto main() -> int {
  // --- Matrix ---

  // DoubleBuffer
  testDoubleBufferMatrix<RowMajorLayout>();
  testDoubleBufferMatrix<ColMajorLayout>();

  // DoubleBuffer::getView
  testDoubleBufferMatrixGetView<RowMajorLayout>();
  testDoubleBufferMatrixGetView<ColMajorLayout>();

  // --- Vector ---

  // DoubleBuffer
  testDoubleBufferVector();

  // DoubleBuffer::getView
  testDoubleBufferVectorGetView();

  // All tests passed!
  return 0;
}

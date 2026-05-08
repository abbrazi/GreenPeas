/// Standard headers
#include <cstdint>
#include <stdexcept>
#include <utility>

/// Helper headers
#include "../Helpers/CountingStorage.hpp"
#include "../Helpers/Macros.hpp"

/// Project headers
#include "GreenPeas/Core/Matrix.hpp"
#include "GreenPeas/Policies/Data/Layout.hpp"
#include "GreenPeas/Policies/Storage/Host.hpp"

using namespace gp;

template <typename Layout>
using HostMatrix = Matrix<uint32_t, uint64_t, HostStorage, Layout>;

template <typename Layout>
using CountingMatrix = Matrix<uint32_t, uint64_t, CountingStorage, Layout>;

// --- MatrixDimensions ---

static void testMatrixDimensionsSize() {
  MatrixDimensions<uint32_t> dims1{2, 3};
  REQUIRE(dims1.size() == 6);
}

static void testMatrixDimensionsEquality() {
  MatrixDimensions<uint32_t> dims1{2, 3};
  MatrixDimensions<uint32_t> dims2{2, 3};
  MatrixDimensions<uint32_t> dims3{2, 4};

  REQUIRE(dims1 == dims2);
  REQUIRE(!(dims1 == dims3));
}

static void testMatrixDimensionsInequality() {
  MatrixDimensions<uint32_t> dims1{2, 3};
  MatrixDimensions<uint32_t> dims2{2, 3};
  MatrixDimensions<uint32_t> dims3{2, 4};

  REQUIRE(dims1 != dims3);
  REQUIRE(!(dims1 != dims2));
}

static void testMatrixDimensionsGreaterThan() {
  MatrixDimensions<uint32_t> dimsSmall{2, 3};
  MatrixDimensions<uint32_t> dimsMax{4, 5};
  MatrixDimensions<uint32_t> dimsEqual{4, 5};
  MatrixDimensions<uint32_t> dimsMoreRows{5, 4};
  MatrixDimensions<uint32_t> dimsMoreCols{4, 6};

  REQUIRE(!(dimsSmall > dimsMax));
  REQUIRE(!(dimsMax > dimsEqual));
  REQUIRE(dimsMoreRows > dimsMax);
  REQUIRE(dimsMoreCols > dimsMax);
}

// --- Matrix  ---

template <typename Layout>
static void testMatrixExplicitConstructorFromDimensions() {
  MatrixDimensions<uint32_t> dims{2, 3};
  HostMatrix<Layout> matrix(dims);
  REQUIRE(matrix.dimensions.numRows == 2);
  REQUIRE(matrix.dimensions.numCols == 3);
  REQUIRE(matrix.maxDimensions.numRows == 2);
  REQUIRE(matrix.maxDimensions.numCols == 3);
  REQUIRE(matrix.data != nullptr);
}

template <typename Layout>
static void testMatrixConstructorFromDimensions() {
  constexpr uint32_t numRows = 4;
  constexpr uint32_t numCols = 5;
  HostMatrix<Layout> matrix(numRows, numCols);
  REQUIRE(matrix.dimensions.numRows == numRows);
  REQUIRE(matrix.dimensions.numCols == numCols);
  REQUIRE(matrix.maxDimensions.numRows == numRows);
  REQUIRE(matrix.maxDimensions.numCols == numCols);
  REQUIRE(matrix.data != nullptr);
}

template <typename Layout>
static void testMatrixDestructor() {
  resetCountingStorage();

  {
    CountingMatrix<Layout> matrix(2, 3);
    (void)matrix;
    REQUIRE(CountingStorage::allocations == 1);
    REQUIRE(CountingStorage::deallocations == 0);
  }

  REQUIRE(CountingStorage::deallocations == 1);
}

template <typename Layout>
static void testMatrixCopyConstructor() {
  resetCountingStorage();

  CountingMatrix<Layout> src(2, 3);
  src(0, 0) = 7;
  src(1, 2) = 42;

  CountingMatrix<Layout> dst(src);

  REQUIRE(dst.dimensions == src.dimensions);
  REQUIRE(dst.maxDimensions == src.dimensions);
  REQUIRE(dst.data != src.data);
  REQUIRE(dst(0, 0) == 7);
  REQUIRE(dst(1, 2) == 42);

  src(0, 0) = 9;
  REQUIRE(dst(0, 0) == 7);
}

template <typename Layout>
static void testMatrixCopyAssignment() {
  resetCountingStorage();

  CountingMatrix<Layout> src(2, 3);
  src(0, 1) = 11;

  CountingMatrix<Layout> dst(4, 5);

  auto deallocationsBefore = CountingStorage::deallocations;
  auto allocationsBefore = CountingStorage::allocations;

  dst = src;

  REQUIRE(dst.dimensions == src.dimensions);
  REQUIRE(dst.maxDimensions == src.dimensions);
  REQUIRE(dst.data != src.data);
  REQUIRE(dst(0, 1) == 11);
  REQUIRE(CountingStorage::deallocations == deallocationsBefore + 1);
  REQUIRE(CountingStorage::allocations == allocationsBefore + 1);

  // Self-assignment should be a no-op.
  // NOLINTNEXTLINE(misc-redundant-expression)
  dst = dst;
  REQUIRE(dst(0, 1) == 11);
}

template <typename Layout>
static void testMatrixMoveConstructor() {
  resetCountingStorage();

  CountingMatrix<Layout> src(2, 3);
  src(1, 1) = 13;

  const auto srcData = src.data;
  const auto srcDims = src.dimensions;
  const auto srcMaxDims = src.maxDimensions;

  CountingMatrix<Layout> dst(std::move(src));

  REQUIRE(dst.data == srcData);
  REQUIRE(dst.dimensions == srcDims);
  REQUIRE(dst.maxDimensions == srcMaxDims);
  REQUIRE(dst(1, 1) == 13);
  REQUIRE(src.data == nullptr);
  constexpr MatrixDimensions<uint32_t> zero{};
  REQUIRE(src.maxDimensions == zero);
  REQUIRE(src.dimensions == zero);
}

template <typename Layout>
static void testMatrixMoveAssignment() {
  resetCountingStorage();

  CountingMatrix<Layout> src(2, 3);
  src(1, 0) = 17;

  const auto srcData = src.data;
  const auto srcDims = src.dimensions;
  const auto srcMaxDims = src.maxDimensions;

  CountingMatrix<Layout> dst(4, 5);

  auto deallocationsBefore = CountingStorage::deallocations;

  dst = std::move(src);

  REQUIRE(dst.data == srcData);
  REQUIRE(dst.dimensions == srcDims);
  REQUIRE(dst.maxDimensions == srcMaxDims);
  REQUIRE(dst(1, 0) == 17);
  REQUIRE(src.data == nullptr);
  constexpr MatrixDimensions<uint32_t> zero{};
  REQUIRE(src.maxDimensions == zero);
  REQUIRE(src.dimensions == zero);
  REQUIRE(CountingStorage::deallocations == deallocationsBefore + 1);
}

template <typename Layout>
static void testMatrixFittoOk() {
  HostMatrix<Layout> matrix(4, 5);
  REQUIRE(matrix.dimensions.numRows == 4);
  REQUIRE(matrix.dimensions.numCols == 5);
  REQUIRE(matrix.maxDimensions.numRows == 4);
  REQUIRE(matrix.maxDimensions.numCols == 5);
  matrix.fitto(2, 3);
  REQUIRE(matrix.dimensions.numRows == 2);
  REQUIRE(matrix.dimensions.numCols == 3);
  REQUIRE(matrix.maxDimensions.numRows == 4);
  REQUIRE(matrix.maxDimensions.numCols == 5);
}

template <typename Layout>
static void testMatrixFittoThrowsWhenExceedsMax() {
  HostMatrix<Layout> matrix(4, 5);
  bool threw = false;
  try {
    matrix.fitto(4, 6);
  } catch (const std::runtime_error &) {
    threw = true;
  }
  REQUIRE(threw);
  REQUIRE(matrix.dimensions.numRows == 4);
  REQUIRE(matrix.dimensions.numCols == 5);
}

template <typename Layout>
static void testMatrixGetElement() {
  HostMatrix<Layout> matrix(4, 5);
  constexpr uint32_t row = 2;
  constexpr uint32_t col = 0;
  const uint32_t idx = Layout::getIndex(row, col, matrix.dimensions);
  matrix.data[idx] = 10;
  REQUIRE(matrix(row, col) == 10);
}

template <typename Layout>
static void testMatrixSetElement() {
  HostMatrix<Layout> matrix(4, 5);
  constexpr uint32_t row = 2;
  constexpr uint32_t col = 0;
  matrix(row, col) = 10;
  const uint32_t idx = Layout::getIndex(row, col, matrix.dimensions);
  REQUIRE(matrix.data[idx] == 10);
}

template <typename Layout>
static void testMatrixConstElementAccess() {
  HostMatrix<Layout> matrix(2, 3);
  matrix(1, 1) = 99;
  const HostMatrix<Layout> &constMatrix = matrix;
  REQUIRE(constMatrix(1, 1) == 99);
}

template <typename Layout>
static void testMatrixCopyToHost() {
  HostMatrix<Layout> matrix1(2, 3);
  matrix1(0, 0) = 1;
  matrix1(1, 2) = 2;
  HostMatrix<Layout> matrix2(2, 3);
  matrix1.copyTo(matrix2);
  REQUIRE(matrix2(0, 0) == 1);
  REQUIRE(matrix2(1, 2) == 2);
}

template <typename Layout>
static void testMatrixCopyToThrowsOnDimensionMismatch() {
  HostMatrix<Layout> matrix1(2, 3);
  HostMatrix<Layout> matrix2(3, 3);
  bool threw = false;
  try {
    matrix1.copyTo(matrix2);
  } catch (const std::runtime_error &) {
    threw = true;
  }
  REQUIRE(threw);
}

template <typename Layout>
static void testMatrixCopyFromHost() {
  HostMatrix<Layout> matrix1(2, 3);
  matrix1(0, 0) = 1;
  matrix1(1, 2) = 2;
  HostMatrix<Layout> matrix2(2, 3);
  matrix2.copyFrom(matrix1);
  REQUIRE(matrix2(0, 0) == 1);
  REQUIRE(matrix2(1, 2) == 2);
}

template <typename Layout>
static void testMatrixCopyFromThrowsOnDimensionMismatch() {
  HostMatrix<Layout> matrix1(2, 3);
  HostMatrix<Layout> matrix2(3, 3);
  bool threw = false;
  try {
    matrix1.copyFrom(matrix2);
  } catch (const std::runtime_error &) {
    threw = true;
  }
  REQUIRE(threw);
}

template <typename Layout>
static void testMatrixSet() {
  HostMatrix<Layout> matrix(2, 2);
  matrix.set();
  REQUIRE(matrix(0, 0) == UINT64_MAX);
  REQUIRE(matrix(0, 1) == UINT64_MAX);
  REQUIRE(matrix(1, 0) == UINT64_MAX);
  REQUIRE(matrix(1, 1) == UINT64_MAX);
}

template <typename Layout>
static void testMatrixClear() {
  HostMatrix<Layout> matrix(2, 2);
  matrix(0, 0) = 1;
  matrix(0, 1) = 2;
  matrix(1, 0) = 3;
  matrix(1, 1) = 4;
  matrix.clear();
  REQUIRE(matrix(0, 0) == 0);
  REQUIRE(matrix(0, 1) == 0);
  REQUIRE(matrix(1, 0) == 0);
  REQUIRE(matrix(1, 1) == 0);
}

template <typename Layout>
static void testMatrixGetView() {
  HostMatrix<Layout> matrix(2, 2);
  matrix(0, 1) = 42;
  auto view = matrix.getView();
  REQUIRE(view.dimensions.numRows == 2);
  REQUIRE(view.dimensions.numCols == 2);
  REQUIRE(view.data == matrix.data);
  // MatrixView::operator() (const)
  REQUIRE(std::as_const(view)(0, 1) == 42);
  // MatrixView::operator() (non-const)
  view(1, 0) = 100;
  REQUIRE(matrix(1, 0) == 100);
  REQUIRE(std::as_const(view)(1, 0) == 100);
}

auto main() -> int {
  // --- MatrixDimensions ---

  // MatrixDimensions::size
  testMatrixDimensionsSize();

  // MatrixDimensions::operator==
  testMatrixDimensionsEquality();

  // MatrixDimensions::operator!=
  testMatrixDimensionsInequality();

  // MatrixDimensions::operator>
  testMatrixDimensionsGreaterThan();

  // --- Matrix ---

  // Constructors
  testMatrixExplicitConstructorFromDimensions<RowMajorLayout>();
  testMatrixExplicitConstructorFromDimensions<ColMajorLayout>();
  testMatrixConstructorFromDimensions<RowMajorLayout>();
  testMatrixConstructorFromDimensions<ColMajorLayout>();

  // Rule of five
  testMatrixDestructor<RowMajorLayout>();
  testMatrixDestructor<ColMajorLayout>();
  testMatrixCopyConstructor<RowMajorLayout>();
  testMatrixCopyConstructor<ColMajorLayout>();
  testMatrixCopyAssignment<RowMajorLayout>();
  testMatrixCopyAssignment<ColMajorLayout>();
  testMatrixMoveConstructor<RowMajorLayout>();
  testMatrixMoveConstructor<ColMajorLayout>();
  testMatrixMoveAssignment<RowMajorLayout>();
  testMatrixMoveAssignment<ColMajorLayout>();

  // Matrix::fitto
  testMatrixFittoOk<RowMajorLayout>();
  testMatrixFittoOk<ColMajorLayout>();
  testMatrixFittoThrowsWhenExceedsMax<RowMajorLayout>();
  testMatrixFittoThrowsWhenExceedsMax<ColMajorLayout>();

  // Matrix::operator() (non-const)
  testMatrixGetElement<RowMajorLayout>();
  testMatrixGetElement<ColMajorLayout>();
  testMatrixSetElement<RowMajorLayout>();
  testMatrixSetElement<ColMajorLayout>();

  // Matrix::operator() (const)
  testMatrixConstElementAccess<RowMajorLayout>();
  testMatrixConstElementAccess<ColMajorLayout>();

  // Matrix::copyTo
  testMatrixCopyToHost<RowMajorLayout>();
  testMatrixCopyToHost<ColMajorLayout>();
  testMatrixCopyToThrowsOnDimensionMismatch<RowMajorLayout>();
  testMatrixCopyToThrowsOnDimensionMismatch<ColMajorLayout>();

  // Matrix::copyFrom
  testMatrixCopyFromHost<RowMajorLayout>();
  testMatrixCopyFromHost<ColMajorLayout>();
  testMatrixCopyFromThrowsOnDimensionMismatch<RowMajorLayout>();
  testMatrixCopyFromThrowsOnDimensionMismatch<ColMajorLayout>();

  // Matrix::set
  testMatrixSet<RowMajorLayout>();
  testMatrixSet<ColMajorLayout>();

  // Matrix::clear
  testMatrixClear<RowMajorLayout>();
  testMatrixClear<ColMajorLayout>();

  // Matrix::getView
  testMatrixGetView<RowMajorLayout>();
  testMatrixGetView<ColMajorLayout>();

  // All tests passed!
  return 0;
}

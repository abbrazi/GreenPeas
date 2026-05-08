/// Standard headers
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>

/// Helper headers
#include "../Helpers/CountingStorage.hpp"
#include "../Helpers/Macros.hpp"

/// Project headers
#include "GreenPeas/Core/SparseMatrix.hpp"
#include "GreenPeas/Policies/Data/Format.hpp"
#include "GreenPeas/Policies/Storage/Host.hpp"

using namespace gp;

using HostSparseMatrix =
    SparseMatrix<uint32_t, uint64_t, HostStorage, COOFormat>;

using CountingSparseMatrix =
    SparseMatrix<uint32_t, uint64_t, CountingStorage, COOFormat>;

// --- SparseMatrixDimensions ---

static void testSparseMatrixDimensionsEquality() {
  SparseMatrixDimensions<uint32_t> dims1{2, 3, 4};
  SparseMatrixDimensions<uint32_t> dims2{2, 3, 4};
  SparseMatrixDimensions<uint32_t> dims3{2, 3, 5};

  REQUIRE(dims1 == dims2);
  REQUIRE(!(dims1 == dims3));
}

static void testSparseMatrixDimensionsInequality() {
  SparseMatrixDimensions<uint32_t> dims1{2, 3, 4};
  SparseMatrixDimensions<uint32_t> dims2{2, 3, 4};
  SparseMatrixDimensions<uint32_t> dims3{2, 3, 5};

  REQUIRE(dims1 != dims3);
  REQUIRE(!(dims1 != dims2));
}

static void testSparseMatrixDimensionsGreaterThan() {
  SparseMatrixDimensions<uint32_t> dimsSmall{2, 3, 4};
  SparseMatrixDimensions<uint32_t> dimsMax{4, 5, 6};
  SparseMatrixDimensions<uint32_t> dimsEqual{4, 5, 6};
  SparseMatrixDimensions<uint32_t> dimsMoreRows{5, 4, 6};
  SparseMatrixDimensions<uint32_t> dimsMoreCols{4, 6, 6};
  SparseMatrixDimensions<uint32_t> dimsMoreNnz{4, 5, 7};

  REQUIRE(!(dimsSmall > dimsMax));
  REQUIRE(!(dimsMax > dimsEqual));
  REQUIRE(dimsMoreRows > dimsMax);
  REQUIRE(dimsMoreCols > dimsMax);
  REQUIRE(dimsMoreNnz > dimsMax);
}

// --- SparseMatrixView ---

static void testSparseMatrixViewDefaultConstruction() {
  SparseMatrixView<uint32_t, uint64_t> view{};
  REQUIRE(view.dimensions.numRows == 0);
  REQUIRE(view.dimensions.numCols == 0);
  REQUIRE(view.dimensions.numNonZeros == 0);
  REQUIRE(view.rows == nullptr);
  REQUIRE(view.cols == nullptr);
  REQUIRE(view.vals == nullptr);
}

// --- SparseMatrix ---

static void testSparseMatrixExplicitConstructorFromDimensions() {
  SparseMatrixDimensions<uint32_t> dims{2, 3, 4};
  HostSparseMatrix matrix(dims);
  REQUIRE(matrix.dimensions.numRows == 2);
  REQUIRE(matrix.dimensions.numCols == 3);
  REQUIRE(matrix.dimensions.numNonZeros == 4);
  REQUIRE(matrix.maxDimensions.numRows == 2);
  REQUIRE(matrix.maxDimensions.numCols == 3);
  REQUIRE(matrix.maxDimensions.numNonZeros == 4);
  REQUIRE(matrix.rows != nullptr);
  REQUIRE(matrix.cols != nullptr);
  REQUIRE(matrix.vals != nullptr);
}

static void testSparseMatrixConstructorFromDimensions() {
  constexpr uint32_t numRows = 4;
  constexpr uint32_t numCols = 5;
  constexpr uint32_t numNonZeros = 3;
  HostSparseMatrix matrix(numRows, numCols, numNonZeros);
  REQUIRE(matrix.dimensions.numRows == numRows);
  REQUIRE(matrix.dimensions.numCols == numCols);
  REQUIRE(matrix.dimensions.numNonZeros == numNonZeros);
  REQUIRE(matrix.maxDimensions.numRows == numRows);
  REQUIRE(matrix.maxDimensions.numCols == numCols);
  REQUIRE(matrix.maxDimensions.numNonZeros == numNonZeros);
  REQUIRE(matrix.rows != nullptr);
  REQUIRE(matrix.cols != nullptr);
  REQUIRE(matrix.vals != nullptr);
}

static void testSparseMatrixDestructor() {
  resetCountingStorage();

  {
    CountingSparseMatrix matrix(2, 3, 4);
    (void)matrix;
    REQUIRE(CountingStorage::allocations == 3);
    REQUIRE(CountingStorage::deallocations == 0);
  }

  REQUIRE(CountingStorage::deallocations == 3);
}

static void testSparseMatrixCopyConstructor() {
  resetCountingStorage();

  CountingSparseMatrix src(2, 3, 4);
  src.rows[0] = 0;
  src.cols[0] = 0;
  src.vals[0] = 7;
  src.rows[2] = 1;
  src.cols[2] = 2;
  src.vals[2] = 42;

  CountingSparseMatrix dst(src);

  REQUIRE(dst.dimensions == src.dimensions);
  REQUIRE(dst.maxDimensions == src.dimensions);
  REQUIRE(dst.rows != src.rows);
  REQUIRE(dst.cols != src.cols);
  REQUIRE(dst.vals != src.vals);
  REQUIRE(dst.rows[0] == 0);
  REQUIRE(dst.cols[0] == 0);
  REQUIRE(dst.vals[0] == 7);
  REQUIRE(dst.rows[2] == 1);
  REQUIRE(dst.cols[2] == 2);
  REQUIRE(dst.vals[2] == 42);

  src.rows[0] = 9;
  src.cols[0] = 9;
  src.vals[0] = 9;
  REQUIRE(dst.rows[0] == 0);
  REQUIRE(dst.cols[0] == 0);
  REQUIRE(dst.vals[0] == 7);
}

static void testSparseMatrixCopyAssignment() {
  resetCountingStorage();

  CountingSparseMatrix src(2, 3, 4);
  src.rows[1] = 0;
  src.cols[1] = 1;
  src.vals[1] = 11;

  CountingSparseMatrix dst(6, 7, 4);

  auto deallocationsBefore = CountingStorage::deallocations;
  auto allocationsBefore = CountingStorage::allocations;

  dst = src;

  REQUIRE(dst.dimensions == src.dimensions);
  REQUIRE(dst.maxDimensions == src.dimensions);
  REQUIRE(dst.rows != src.rows);
  REQUIRE(dst.cols != src.cols);
  REQUIRE(dst.vals != src.vals);
  REQUIRE(dst.rows[1] == 0);
  REQUIRE(dst.cols[1] == 1);
  REQUIRE(dst.vals[1] == 11);
  REQUIRE(CountingStorage::deallocations == deallocationsBefore + 3);
  REQUIRE(CountingStorage::allocations == allocationsBefore + 3);

  // Self-assignment should be a no-op.
  // NOLINTNEXTLINE(misc-redundant-expression)
  dst = dst;
  REQUIRE(dst.vals[1] == 11);
}

static void testSparseMatrixMoveConstructor() {
  resetCountingStorage();

  CountingSparseMatrix src(2, 3, 4);
  src.vals[2] = 13;

  const auto srcRows = src.rows;
  const auto srcCols = src.cols;
  const auto srcVals = src.vals;
  const auto srcDims = src.dimensions;
  const auto srcMaxDims = src.maxDimensions;

  CountingSparseMatrix dst(std::move(src));

  REQUIRE(dst.rows == srcRows);
  REQUIRE(dst.cols == srcCols);
  REQUIRE(dst.vals == srcVals);
  REQUIRE(dst.dimensions == srcDims);
  REQUIRE(dst.maxDimensions == srcMaxDims);
  REQUIRE(dst.vals[2] == 13);
  REQUIRE(src.rows == nullptr);
  REQUIRE(src.cols == nullptr);
  REQUIRE(src.vals == nullptr);
  constexpr SparseMatrixDimensions<uint32_t> zero{};
  REQUIRE(src.maxDimensions == zero);
  REQUIRE(src.dimensions == zero);
}

static void testSparseMatrixMoveAssignment() {
  resetCountingStorage();

  CountingSparseMatrix src(2, 3, 4);
  src.vals[0] = 17;

  const auto srcRows = src.rows;
  const auto srcCols = src.cols;
  const auto srcVals = src.vals;
  const auto srcDims = src.dimensions;
  const auto srcMaxDims = src.maxDimensions;

  CountingSparseMatrix dst(6, 7, 4);

  auto deallocationsBefore = CountingStorage::deallocations;

  dst = std::move(src);

  REQUIRE(dst.rows == srcRows);
  REQUIRE(dst.cols == srcCols);
  REQUIRE(dst.vals == srcVals);
  REQUIRE(dst.dimensions == srcDims);
  REQUIRE(dst.maxDimensions == srcMaxDims);
  REQUIRE(dst.vals[0] == 17);
  REQUIRE(src.rows == nullptr);
  REQUIRE(src.cols == nullptr);
  REQUIRE(src.vals == nullptr);
  constexpr SparseMatrixDimensions<uint32_t> zero{};
  REQUIRE(src.maxDimensions == zero);
  REQUIRE(src.dimensions == zero);
  REQUIRE(CountingStorage::deallocations == deallocationsBefore + 3);
}

static void testSparseMatrixFittoOk() {
  HostSparseMatrix matrix(4, 5, 6);
  REQUIRE(matrix.dimensions.numRows == 4);
  REQUIRE(matrix.dimensions.numCols == 5);
  REQUIRE(matrix.dimensions.numNonZeros == 6);
  REQUIRE(matrix.maxDimensions.numRows == 4);
  REQUIRE(matrix.maxDimensions.numCols == 5);
  REQUIRE(matrix.maxDimensions.numNonZeros == 6);
  matrix.fitto(2, 3, 4);
  REQUIRE(matrix.dimensions.numRows == 2);
  REQUIRE(matrix.dimensions.numCols == 3);
  REQUIRE(matrix.dimensions.numNonZeros == 4);
  REQUIRE(matrix.maxDimensions.numRows == 4);
  REQUIRE(matrix.maxDimensions.numCols == 5);
  REQUIRE(matrix.maxDimensions.numNonZeros == 6);
}

static void testSparseMatrixFittoThrowsWhenExceedsMax() {
  HostSparseMatrix matrix(4, 5, 3);
  bool threw = false;
  try {
    matrix.fitto(4, 6, 3);
  } catch (const std::runtime_error &) {
    threw = true;
  }
  REQUIRE(threw);
  REQUIRE(matrix.dimensions.numRows == 4);
  REQUIRE(matrix.dimensions.numCols == 5);
  REQUIRE(matrix.dimensions.numNonZeros == 3);
}

static void testSparseMatrixCopyToHost() {
  HostSparseMatrix matrix1(2, 3, 4);
  matrix1.rows[0] = 0;
  matrix1.cols[0] = 0;
  matrix1.vals[0] = 1;
  matrix1.rows[2] = 1;
  matrix1.cols[2] = 2;
  matrix1.vals[2] = 2;
  HostSparseMatrix matrix2(2, 3, 4);
  matrix1.copyTo(matrix2);
  REQUIRE(matrix2.rows[0] == 0);
  REQUIRE(matrix2.cols[0] == 0);
  REQUIRE(matrix2.vals[0] == 1);
  REQUIRE(matrix2.rows[2] == 1);
  REQUIRE(matrix2.cols[2] == 2);
  REQUIRE(matrix2.vals[2] == 2);
}

static void testSparseMatrixCopyToThrowsOnDimensionMismatch() {
  HostSparseMatrix matrix1(2, 3, 3);
  HostSparseMatrix matrix2(3, 3, 3);
  bool threw = false;
  try {
    matrix1.copyTo(matrix2);
  } catch (const std::runtime_error &) {
    threw = true;
  }
  REQUIRE(threw);
}

static void testSparseMatrixCopyFromHost() {
  HostSparseMatrix matrix1(2, 3, 4);
  matrix1.rows[0] = 0;
  matrix1.cols[0] = 0;
  matrix1.vals[0] = 1;
  matrix1.rows[2] = 1;
  matrix1.cols[2] = 2;
  matrix1.vals[2] = 2;
  HostSparseMatrix matrix2(2, 3, 4);
  matrix2.copyFrom(matrix1);
  REQUIRE(matrix2.rows[0] == 0);
  REQUIRE(matrix2.cols[0] == 0);
  REQUIRE(matrix2.vals[0] == 1);
  REQUIRE(matrix2.rows[2] == 1);
  REQUIRE(matrix2.cols[2] == 2);
  REQUIRE(matrix2.vals[2] == 2);
}

static void testSparseMatrixCopyFromThrowsOnDimensionMismatch() {
  HostSparseMatrix matrix1(2, 3, 3);
  HostSparseMatrix matrix2(3, 3, 3);
  bool threw = false;
  try {
    matrix1.copyFrom(matrix2);
  } catch (const std::runtime_error &) {
    threw = true;
  }
  REQUIRE(threw);
}

static void testSparseMatrixGetView() {
  HostSparseMatrix matrix(2, 2, 2);
  matrix.rows[0] = 0;
  matrix.cols[0] = 1;
  matrix.vals[0] = 42;
  auto view = matrix.getView();
  REQUIRE(view.dimensions.numRows == 2);
  REQUIRE(view.dimensions.numCols == 2);
  REQUIRE(view.dimensions.numNonZeros == 2);
  REQUIRE(view.rows == matrix.rows);
  REQUIRE(view.cols == matrix.cols);
  REQUIRE(view.vals == matrix.vals);
  REQUIRE(view.vals[0] == 42);
  view.vals[1] = 100;
  REQUIRE(matrix.vals[1] == 100);
}

template <size_t N>
static void requireMatrixElements(const HostSparseMatrix &matrix,
                                  uint32_t numRows,
                                  uint32_t numCols,
                                  const uint32_t (&expectedRows)[N],
                                  const uint32_t (&expectedCols)[N],
                                  const uint64_t (&expectedVals)[N]) {
  REQUIRE(matrix.dimensions.numRows == numRows);
  REQUIRE(matrix.dimensions.numCols == numCols);
  REQUIRE(matrix.dimensions.numNonZeros == N);
  for (size_t i = 0; i < N; ++i) {
    REQUIRE(matrix.rows[i] == expectedRows[i]);
    REQUIRE(matrix.cols[i] == expectedCols[i]);
    REQUIRE(matrix.vals[i] == expectedVals[i]);
  }
}

static void testSparseMatrixFromMtx() {
  const std::string base = std::string(GP_DATA_PATH) + "/codes/surface/d3/";

  {
    const HostSparseMatrix matrix = HostSparseMatrix::fromMtx(base + "Lx.mtx");
    const uint32_t rows[] = {0, 0, 0};
    const uint32_t cols[] = {6, 7, 8};
    const uint64_t vals[] = {1, 1, 1};
    requireMatrixElements(matrix, 1, 9, rows, cols, vals);
  }

  {
    const HostSparseMatrix matrix = HostSparseMatrix::fromMtx(base + "Lz.mtx");
    const uint32_t rows[] = {0, 0, 0};
    const uint32_t cols[] = {0, 4, 8};
    const uint64_t vals[] = {1, 1, 1};
    requireMatrixElements(matrix, 1, 9, rows, cols, vals);
  }

  {
    const HostSparseMatrix matrix = HostSparseMatrix::fromMtx(base + "Hx.mtx");
    const uint32_t rows[] = {0, 2, 3, 0, 2, 3, 0, 1, 3, 0, 1, 3};
    const uint32_t cols[] = {4, 6, 8, 1, 3, 5, 3, 5, 7, 0, 2, 4};
    const uint64_t vals[] = {0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3};
    requireMatrixElements(matrix, 4, 9, rows, cols, vals);
  }

  {
    const HostSparseMatrix matrix = HostSparseMatrix::fromMtx(base + "Hz.mtx");
    const uint32_t rows[] = {0, 1, 2, 0, 1, 2, 1, 2, 3, 1, 2, 3};
    const uint32_t cols[] = {1, 5, 7, 0, 4, 6, 2, 4, 8, 1, 3, 7};
    const uint64_t vals[] = {0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3};
    requireMatrixElements(matrix, 4, 9, rows, cols, vals);
  }
}

auto main() -> int {
  // --- SparseMatrixDimensions ---

  // SparseMatrixDimensions::operator==
  testSparseMatrixDimensionsEquality();

  // SparseMatrixDimensions::operator!=
  testSparseMatrixDimensionsInequality();

  // SparseMatrixDimensions::operator>
  testSparseMatrixDimensionsGreaterThan();

  // --- SparseMatrixView ---

  // Constructor
  testSparseMatrixViewDefaultConstruction();

  // --- SparseMatrix ---

  // Constructors
  testSparseMatrixExplicitConstructorFromDimensions();
  testSparseMatrixConstructorFromDimensions();

  // Rule of five
  testSparseMatrixDestructor();
  testSparseMatrixCopyConstructor();
  testSparseMatrixCopyAssignment();
  testSparseMatrixMoveConstructor();
  testSparseMatrixMoveAssignment();

  // SparseMatrix::fitto
  testSparseMatrixFittoOk();
  testSparseMatrixFittoThrowsWhenExceedsMax();

  // SparseMatrix::copyTo
  testSparseMatrixCopyToHost();
  testSparseMatrixCopyToThrowsOnDimensionMismatch();

  // SparseMatrix::copyFrom
  testSparseMatrixCopyFromHost();
  testSparseMatrixCopyFromThrowsOnDimensionMismatch();

  // SparseMatrix::getView
  testSparseMatrixGetView();

  // SparseMatrix::fromMtx
  testSparseMatrixFromMtx();

  // All tests passed!
  return 0;
}

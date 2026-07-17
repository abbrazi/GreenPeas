#ifndef GREENPEAS_CORE_SPARSEMATRIX_HPP
#define GREENPEAS_CORE_SPARSEMATRIX_HPP

/// Standard headers
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

/// Project headers
#include "GreenPeas/Common.hpp"

namespace gp {

/// @brief Sparse matrix shape (`numRows`, `numCols`, `numNonZeros`).
/// @tparam IndexT Index type.
template <typename IndexT>
struct SparseMatrixDimensions {
  /// @brief Number of rows (`numRows`).
  IndexT numRows;

  /// @brief Number of columns (`numCols`).
  IndexT numCols;

  /// @brief Number of stored non-zeros (`numNonZeros`).
  IndexT numNonZeros;

  /// @brief Equality comparison.
  /// @param other Dimensions to compare against.
  /// @return True if equal to `other`.
  HOST auto operator==(const SparseMatrixDimensions &other) const -> bool {
    return numRows == other.numRows && numCols == other.numCols &&
           numNonZeros == other.numNonZeros;
  }

  /// @brief Inequality comparison.
  /// @param other Dimensions to compare against.
  /// @return True if not equal to `other`.
  HOST auto operator!=(const SparseMatrixDimensions &other) const -> bool {
    return !(*this == other);
  }

  /// @brief Greater-than comparison.
  /// @param other Dimensions to compare against.
  /// @return True if greater than `other`.
  HOST DEVICE auto operator>(const SparseMatrixDimensions &other) const
      -> bool {
    return numRows > other.numRows || numCols > other.numCols ||
           numNonZeros > other.numNonZeros;
  }
};

/// @brief Non-owning view over sparse matrix arrays.
/// @tparam IndexT Index type.
/// @tparam ValueT Value type.
template <typename IndexT, typename ValueT>
struct SparseMatrixView {
  /// @brief Matrix shape.
  SparseMatrixDimensions<IndexT> dimensions;

  /// @brief Backing C-array for row indices.
  IndexT *rows;

  /// @brief Backing C-array for column indices.
  IndexT *cols;

  /// @brief Backing C-array for values.
  ValueT *vals;
};

/// @brief Owning sparse matrix with configurable storage and format.
/// @tparam IndexT Index type.
/// @tparam ValueT Value type.
/// @tparam Storage Storage policy for allocation/copy.
/// @tparam Format Sparse format policy for array sizing.
template <typename IndexT, typename ValueT, typename Storage, typename Format>
struct SparseMatrix {
  /// @brief Maximum dimensions. Invariant: dimensions <= maxDimensions.
  SparseMatrixDimensions<IndexT> maxDimensions;

  /// @brief Current dimensions. Must be <= maxDimensions.
  SparseMatrixDimensions<IndexT> dimensions;

  /// @brief Backing C-array for row indices.
  IndexT *rows;

  /// @brief Backing C-array for column indices.
  IndexT *cols;

  /// @brief Backing C-array for values.
  ValueT *vals;

  /// @brief Construct a sparse matrix from:
  /// @param dims Initial dimensions (also max dimensions).
  HOST explicit SparseMatrix(const SparseMatrixDimensions<IndexT> &dims)
      : maxDimensions(dims), dimensions(maxDimensions) {
    const IndexT rowsSize =
        Format::template getRowsArraySize<IndexT>(maxDimensions);
    const IndexT colsSize =
        Format::template getColsArraySize<IndexT>(maxDimensions);
    const IndexT valsSize =
        Format::template getValsArraySize<IndexT>(maxDimensions);
    rows = Storage::template allocate<IndexT>(rowsSize);
    cols = Storage::template allocate<IndexT>(colsSize);
    vals = Storage::template allocate<ValueT>(valsSize);
  }

  /// @brief Construct a sparse matrix from:
  /// @param numRows Initial number of rows.
  /// @param numCols Initial number of columns.
  /// @param numNonZeros Initial number of non-zeros.
  HOST SparseMatrix(IndexT numRows, IndexT numCols, IndexT numNonZeros)
      : SparseMatrix(
            SparseMatrixDimensions<IndexT>{numRows, numCols, numNonZeros}) {}

  /// @brief Destructor.
  HOST ~SparseMatrix() {
    Storage::deallocate(rows);
    Storage::deallocate(cols);
    Storage::deallocate(vals);
  }

  /// @brief Copy constructor.
  /// @param other Sparse matrix to copy from.
  HOST SparseMatrix(const SparseMatrix &other)
      : maxDimensions(other.dimensions), dimensions(maxDimensions) {
    const IndexT rowsSize =
        Format::template getRowsArraySize<IndexT>(dimensions);
    const IndexT colsSize =
        Format::template getColsArraySize<IndexT>(dimensions);
    const IndexT valsSize =
        Format::template getValsArraySize<IndexT>(dimensions);
    rows = Storage::template allocate<IndexT>(rowsSize);
    cols = Storage::template allocate<IndexT>(colsSize);
    vals = Storage::template allocate<ValueT>(valsSize);
    Storage::copyFrom(rows, other.rows, rowsSize, Storage{});
    Storage::copyFrom(cols, other.cols, colsSize, Storage{});
    Storage::copyFrom(vals, other.vals, valsSize, Storage{});
  }

  /// @brief Copy assignment.
  /// @param other Sparse matrix to copy from.
  /// @return Reference to `*this`.
  HOST auto operator=(const SparseMatrix &other) -> SparseMatrix & {
    if (this != &other) {
      Storage::deallocate(rows);
      Storage::deallocate(cols);
      Storage::deallocate(vals);
      maxDimensions = other.dimensions;
      dimensions = maxDimensions;
      const IndexT rowsSize =
          Format::template getRowsArraySize<IndexT>(dimensions);
      const IndexT colsSize =
          Format::template getColsArraySize<IndexT>(dimensions);
      const IndexT valsSize =
          Format::template getValsArraySize<IndexT>(dimensions);
      rows = Storage::template allocate<IndexT>(rowsSize);
      cols = Storage::template allocate<IndexT>(colsSize);
      vals = Storage::template allocate<ValueT>(valsSize);
      Storage::copyFrom(rows, other.rows, rowsSize, Storage{});
      Storage::copyFrom(cols, other.cols, colsSize, Storage{});
      Storage::copyFrom(vals, other.vals, valsSize, Storage{});
    }
    return *this;
  }

  /// @brief Move constructor.
  /// @param other Sparse matrix to move from.
  HOST SparseMatrix(SparseMatrix &&other) noexcept
      : maxDimensions(other.maxDimensions), dimensions(maxDimensions),
        rows(other.rows), cols(other.cols), vals(other.vals) {
    other.rows = nullptr;
    other.cols = nullptr;
    other.vals = nullptr;
    other.maxDimensions = SparseMatrixDimensions<IndexT>{};
    other.dimensions = SparseMatrixDimensions<IndexT>{};
  }

  /// @brief Move assignment.
  /// @param other Sparse matrix to move from.
  /// @return Reference to `*this`.
  HOST auto operator=(SparseMatrix &&other) noexcept -> SparseMatrix & {
    if (this != &other) {
      Storage::deallocate(rows);
      Storage::deallocate(cols);
      Storage::deallocate(vals);
      maxDimensions = other.maxDimensions;
      dimensions = maxDimensions;
      rows = other.rows;
      cols = other.cols;
      vals = other.vals;
      other.rows = nullptr;
      other.cols = nullptr;
      other.vals = nullptr;
      other.maxDimensions = SparseMatrixDimensions<IndexT>{};
      other.dimensions = SparseMatrixDimensions<IndexT>{};
    }
    return *this;
  }

  /// @brief Fit new dimensions (must be <= maxDimensions).
  /// @param newDimensions New dimensions.
  HOST void fitto(const SparseMatrixDimensions<IndexT> &newDimensions) {
    if (newDimensions > maxDimensions) {
      throw std::runtime_error(
          "SparseMatrix: newDimensions exceeds maxDimensions.");
    }
    dimensions = newDimensions;
  }

  /// @brief Fit new dimensions (must be <= maxDimensions).
  /// @param numRows New number of rows.
  /// @param numCols New number of columns.
  /// @param numNonZeros New number of non-zeros.
  HOST void fitto(IndexT numRows, IndexT numCols, IndexT numNonZeros) {
    SparseMatrixDimensions<IndexT> newDimensions{numRows, numCols, numNonZeros};
    fitto(newDimensions);
  }

  /// @brief Copy to another sparse matrix.
  /// @tparam DestinationStorage Destination storage policy.
  /// @param other Destination sparse matrix (must have same dimensions).
  template <typename DestinationStorage>
  HOST void copyTo(
      SparseMatrix<IndexT, ValueT, DestinationStorage, Format> &other) const {
    if (dimensions != other.dimensions) {
      throw std::runtime_error("SparseMatrix: dimensions mismatch in copyTo.");
    }
    const IndexT rowsSize =
        Format::template getRowsArraySize<IndexT>(dimensions);
    const IndexT colsSize =
        Format::template getColsArraySize<IndexT>(dimensions);
    const IndexT valsSize =
        Format::template getValsArraySize<IndexT>(dimensions);
    Storage::copyTo(other.rows, rows, rowsSize, DestinationStorage{});
    Storage::copyTo(other.cols, cols, colsSize, DestinationStorage{});
    Storage::copyTo(other.vals, vals, valsSize, DestinationStorage{});
  }

  /// @brief Copy from another sparse matrix.
  /// @tparam SourceStorage Source storage policy.
  /// @param other Source sparse matrix (must have same dimensions).
  template <typename SourceStorage>
  HOST void
  copyFrom(const SparseMatrix<IndexT, ValueT, SourceStorage, Format> &other) {
    if (dimensions != other.dimensions) {
      throw std::runtime_error(
          "SparseMatrix: dimensions mismatch in copyFrom.");
    }
    const IndexT rowsSize =
        Format::template getRowsArraySize<IndexT>(dimensions);
    const IndexT colsSize =
        Format::template getColsArraySize<IndexT>(dimensions);
    const IndexT valsSize =
        Format::template getValsArraySize<IndexT>(dimensions);
    Storage::copyFrom(rows, other.rows, rowsSize, SourceStorage{});
    Storage::copyFrom(cols, other.cols, colsSize, SourceStorage{});
    Storage::copyFrom(vals, other.vals, valsSize, SourceStorage{});
  }

  /// @brief Get a non-owning view of the sparse matrix.
  /// @return SparseMatrixView sharing this matrix's storage.
  HOST auto getView() -> SparseMatrixView<IndexT, ValueT> {
    return {dimensions, rows, cols, vals};
  }

  /// @brief Load a Matrix Market (`.mtx`) file into a COUP matrix.
  /// @param path Path to the `.mtx` file.
  /// @return Sparse matrix with entries in file order (0-based indices).
  /// @note Only available when `Format` is `COOFormat`.
  HOST static auto fromMtx(const std::string &path) -> SparseMatrix {
    static_assert(Format::isCoordinate,
                  "SparseMatrix::fromMtx is only supported for COOFormat.");

    std::ifstream file(path);
    if (!file.is_open()) {
      throw std::runtime_error("Could not open file: " + path);
    }

    std::string line;
    while (std::getline(file, line)) {
      if (!line.empty() && line[0] != '%') {
        break;
      }
    }

    std::stringstream stream(line);
    IndexT numRows{};
    IndexT numCols{};
    IndexT numNonZeros{};
    stream >> numRows >> numCols >> numNonZeros;

    SparseMatrix matrix(numRows, numCols, numNonZeros);

    IndexT row{};
    IndexT col{};
    ValueT val{};
    IndexT nonZero = 0;
    while (nonZero < numNonZeros && (file >> row >> col >> val)) {
      matrix.rows[nonZero] = row - 1;
      matrix.cols[nonZero] = col - 1;
      matrix.vals[nonZero] = val;
      ++nonZero;
    }

    return matrix;
  }
};

} // namespace gp

#endif // GREENPEAS_CORE_SPARSEMATRIX_HPP

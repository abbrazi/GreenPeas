#ifndef GREENPEAS_CORE_MATRIX_HPP
#define GREENPEAS_CORE_MATRIX_HPP

/// Standard headers
#include <stdexcept>

/// Project headers
#include "GreenPeas/Common.hpp"

namespace gp {

/// @brief Matrix shape (`numRows` x `numCols`).
/// @tparam IndexT Index type.
template <typename IndexT>
struct MatrixDimensions {
  /// @brief Number of rows (`numRows`).
  IndexT numRows;

  /// @brief Number of columns (`numCols`).
  IndexT numCols;

  /// @brief Total number of elements.
  /// @return `numRows * numCols`.
  HOST DEVICE constexpr auto size() const -> IndexT {
    return numRows * numCols;
  }

  /// @brief Equality comparison.
  /// @param other Dimensions to compare against.
  /// @return True if equal to `other`.
  HOST DEVICE auto operator==(const MatrixDimensions &other) const -> bool {
    return numRows == other.numRows && numCols == other.numCols;
  }

  /// @brief Inequality comparison.
  /// @param other Dimensions to compare against.
  /// @return True if not equal to `other`.
  HOST DEVICE auto operator!=(const MatrixDimensions &other) const -> bool {
    return !(*this == other);
  }

  /// @brief Greater-than comparison.
  /// @param other Dimensions to compare against.
  /// @return True if greater than `other`.
  HOST DEVICE auto operator>(const MatrixDimensions &other) const -> bool {
    return numRows > other.numRows || numCols > other.numCols;
  }
};

/// @brief Non-owning view over matrix data.
/// @tparam IndexT Index type.
/// @tparam ValueT Value type.
/// @tparam Layout Layout policy with getIndex(row, col, dimensions).
template <typename IndexT, typename ValueT, typename Layout>
struct MatrixView {
  /// @brief Matrix shape.
  MatrixDimensions<IndexT> dimensions;

  /// @brief Backing C-array.
  ValueT *data;

  /// @brief Element access (non-const).
  /// @param row Row index in `[0, dimensions.numRows)`.
  /// @param col Column index in `[0, dimensions.numCols)`.
  /// @return Reference to the element at (row, col).
  HOST DEVICE auto operator()(IndexT row, IndexT col) -> ValueT & {
    return data[Layout::getIndex(row, col, dimensions)];
  }

  /// @brief Element access (const view).
  /// @param row Row index in `[0, dimensions.numRows)`.
  /// @param col Column index in `[0, dimensions.numCols)`.
  /// @return Reference to the element at (row, col).
  HOST DEVICE auto operator()(IndexT row, IndexT col) const -> ValueT & {
    return data[Layout::getIndex(row, col, dimensions)];
  }
};

/// @brief Owning matrix with configurable storage and layout.
/// @tparam IndexT Index type.
/// @tparam ValueT Value type.
/// @tparam Storage Storage policy for allocation/copy.
/// @tparam Layout Layout policy with getIndex(row, col, dimensions).
template <typename IndexT, typename ValueT, typename Storage, typename Layout>
struct Matrix {
  /// @brief Maximum dimensions. Invariant: dimensions <= maxDimensions.
  MatrixDimensions<IndexT> maxDimensions;

  /// @brief Current dimensions. Must be <= maxDimensions.
  MatrixDimensions<IndexT> dimensions;

  /// @brief Backing C-array.
  ValueT *data;

  /// @brief Constructs a matrix from dimensions.
  /// @param dims Initial dimensions (also max dimensions).
  HOST explicit Matrix(const MatrixDimensions<IndexT> &dims)
      : maxDimensions(dims), dimensions(maxDimensions) {
    data = Storage::template allocate<ValueT>(maxDimensions.size());
  }

  /// @brief Constructs a matrix from number of rows and columns.
  /// @param numRows Initial number of rows.
  /// @param numCols Initial number of columns.
  HOST Matrix(IndexT numRows, IndexT numCols)
      : Matrix(MatrixDimensions<IndexT>{numRows, numCols}) {}

  /// @brief Destructor.
  HOST ~Matrix() { Storage::deallocate(data); }

  /// @brief Copy constructor.
  HOST Matrix(const Matrix &other)
      : maxDimensions(other.dimensions), dimensions(maxDimensions) {
    const IndexT size = dimensions.size();
    data = Storage::template allocate<ValueT>(size);
    Storage::copyFrom(data, other.data, size, Storage{});
  }

  /// @brief Copy assignment.
  HOST auto operator=(const Matrix &other) -> Matrix & {
    if (this != &other) {
      Storage::deallocate(data);
      maxDimensions = other.dimensions;
      dimensions = maxDimensions;
      data = Storage::template allocate<ValueT>(dimensions.size());
      Storage::copyFrom(data, other.data, dimensions.size(), Storage{});
    }
    return *this;
  }

  /// @brief Move constructor.
  HOST Matrix(Matrix &&other) noexcept
      : maxDimensions(other.maxDimensions), dimensions(maxDimensions),
        data(other.data) {
    other.data = nullptr;
    other.maxDimensions = MatrixDimensions<IndexT>{};
    other.dimensions = MatrixDimensions<IndexT>{};
  }

  /// @brief Move assignment.
  HOST auto operator=(Matrix &&other) noexcept -> Matrix & {
    if (this != &other) {
      Storage::deallocate(data);
      maxDimensions = other.maxDimensions;
      dimensions = maxDimensions;
      data = other.data;
      other.data = nullptr;
      other.maxDimensions = MatrixDimensions<IndexT>{};
      other.dimensions = MatrixDimensions<IndexT>{};
    }
    return *this;
  }

  /// @brief Fit new dimensions (must be <= maxDimensions).
  /// @param newDimensions New dimensions.
  HOST void fitto(const MatrixDimensions<IndexT> &newDimensions) {
    if (newDimensions > maxDimensions) {
      throw std::runtime_error("Matrix: newDimensions exceeds maxDimensions.");
    }
    dimensions = newDimensions;
  }

  /// @brief Fit new dimensions (must be <= maxDimensions).
  /// @param numRows New number of rows.
  /// @param numCols New number of columns.
  HOST void fitto(IndexT numRows, IndexT numCols) {
    MatrixDimensions<IndexT> newDimensions(numRows, numCols);
    fitto(newDimensions);
  }

  /// @brief Element access (non-const).
  /// @param row Row index in `[0, dimensions.numRows)`.
  /// @param col Column index in `[0, dimensions.numCols)`.
  /// @return Reference to the element at (row, col).
  HOST auto operator()(IndexT row, IndexT col) -> ValueT & {
    return data[Layout::getIndex(row, col, dimensions)];
  }

  /// @brief Element access (const).
  /// @param row Row index in `[0, dimensions.numRows)`.
  /// @param col Column index in `[0, dimensions.numCols)`.
  /// @return Value at (row, col).
  HOST auto operator()(IndexT row, IndexT col) const -> ValueT {
    return data[Layout::getIndex(row, col, dimensions)];
  }

  /// @brief Copy to another matrix.
  /// @tparam DestinationStorage Destination storage policy.
  /// @param other Destination matrix (must have same dimensions).
  template <typename DestinationStorage>
  HOST void
  copyTo(Matrix<IndexT, ValueT, DestinationStorage, Layout> &other) const {
    if (dimensions != other.dimensions) {
      throw std::runtime_error("Matrix: dimensions mismatch in copyTo.");
    }
    Storage::copyTo(other.data, data, dimensions.size(), DestinationStorage{});
  }

  /// @brief Copy from another matrix.
  /// @tparam SourceStorage Source storage policy.
  /// @param other Source matrix (must have same dimensions).
  template <typename SourceStorage>
  HOST void
  copyFrom(const Matrix<IndexT, ValueT, SourceStorage, Layout> &other) {
    if (dimensions != other.dimensions) {
      throw std::runtime_error("Matrix: dimensions mismatch in copyFrom.");
    }
    Storage::copyFrom(data, other.data, dimensions.size(), SourceStorage{});
  }

  /// @brief Set all elements.
  HOST void set() { Storage::set(data, dimensions.size()); }

  /// @brief Clear all elements.
  HOST void clear() { Storage::clear(data, dimensions.size()); }

  /// @brief Get a non-owning view of the matrix.
  /// @return MatrixView sharing this matrix's storage.
  HOST auto getView() -> MatrixView<IndexT, ValueT, Layout> {
    return {dimensions, data};
  }
};

} // namespace gp

#endif // GREENPEAS_CORE_MATRIX_HPP

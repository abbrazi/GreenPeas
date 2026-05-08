#ifndef GREENPEAS_POLICIES_DATA_LAYOUT_HPP
#define GREENPEAS_POLICIES_DATA_LAYOUT_HPP

/// Project headers
#include "GreenPeas/Common.hpp"
#include "GreenPeas/Core/Matrix.hpp"

namespace gp {

/// @brief Row-major dense matrix layout.
struct RowMajorLayout {
  /// @brief Get the linear index for `(row, col)`.
  /// @tparam IndexT Index type.
  /// @param row Row index.
  /// @param col Column index.
  /// @param dimensions Matrix dimensions.
  /// @return `(row * dimensions.numCols) + col`.
  template <typename IndexT>
  HOST DEVICE static constexpr auto
  getIndex(IndexT row, IndexT col, MatrixDimensions<IndexT> dimensions)
      -> IndexT {
    return (row * dimensions.numCols) + col;
  }
};

/// @brief Column-major dense matrix layout.
struct ColMajorLayout {
  /// @brief Get the linear index for `(row, col)`.
  /// @tparam IndexT Index type.
  /// @param row Row index.
  /// @param col Column index.
  /// @param dimensions Matrix dimensions.
  /// @return `(col * dimensions.numRows) + row`.
  template <typename IndexT>
  HOST DEVICE static constexpr auto
  getIndex(IndexT row, IndexT col, MatrixDimensions<IndexT> dimensions)
      -> IndexT {
    return (col * dimensions.numRows) + row;
  }
};

} // namespace gp

#endif // GREENPEAS_POLICIES_DATA_LAYOUT_HPP

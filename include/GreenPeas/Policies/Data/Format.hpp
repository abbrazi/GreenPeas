#ifndef GREENPEAS_POLICIES_DATA_FORMAT_HPP
#define GREENPEAS_POLICIES_DATA_FORMAT_HPP

/// Project headers
#include "GreenPeas/Common.hpp"
#include "GreenPeas/Core/SparseMatrix.hpp"

namespace gp {

/// @brief COUP (coordinate) sparse matrix format.
struct COOFormat {
  /// @brief Can convert from Matrix Market (.mtx) file.
  static constexpr bool isCoordinate = true;

  /// @brief Get the size of the `rows` array.
  /// @tparam IndexT Index type.
  /// @param dimensions Sparse matrix dimensions.
  /// @return `dimensions.numNonZeros`.
  template <typename IndexT>
  HOST DEVICE static constexpr auto
  getRowsArraySize(SparseMatrixDimensions<IndexT> dimensions) -> IndexT {
    return dimensions.numNonZeros;
  }

  /// @brief Get the size of the `cols` array.
  /// @tparam IndexT Index type.
  /// @param dimensions Sparse matrix dimensions.
  /// @return `dimensions.numNonZeros`.
  template <typename IndexT>
  HOST DEVICE static constexpr auto
  getColsArraySize(SparseMatrixDimensions<IndexT> dimensions) -> IndexT {
    return dimensions.numNonZeros;
  }

  /// @brief Get the size of the `vals` array.
  /// @tparam IndexT Index type.
  /// @param dimensions Sparse matrix dimensions.
  /// @return `dimensions.numNonZeros`.
  template <typename IndexT>
  HOST DEVICE static constexpr auto
  getValsArraySize(SparseMatrixDimensions<IndexT> dimensions) -> IndexT {
    return dimensions.numNonZeros;
  }
};

} // namespace gp

#endif // GREENPEAS_POLICIES_DATA_FORMAT_HPP

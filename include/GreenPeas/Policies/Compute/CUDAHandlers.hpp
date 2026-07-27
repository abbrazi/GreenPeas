#ifndef GREENPEAS_POLICIES_COMPUTE_CUDAHANDLERS_HPP
#define GREENPEAS_POLICIES_COMPUTE_CUDAHANDLERS_HPP

/// CUDA headers
#include <thrust/tuple.h>

/// Project headers
#include "GreenPeas/Common.hpp"
#include "GreenPeas/Core/Array.hpp"
#include "GreenPeas/Core/Graph.hpp"
#include "GreenPeas/Core/Matrix.hpp"
#include "GreenPeas/Core/Scalar.hpp"
#include "GreenPeas/Core/Vector.hpp"
#include "GreenPeas/Core/Words.hpp"
#include "GreenPeas/Policies/Data/Layout.hpp"

namespace gp {

/// @brief Non-owning CUDA graph view.
using CUDAGraphView = GraphView;

/// @brief Non-owning CUDA matrix view (column-major).
/// @tparam ValueT Value type.
template <typename ValueT>
using CUDAMatrixView = MatrixView<uint32_t, ValueT, ColMajorLayout>;

/// @brief Non-owning CUDA scalar view.
/// @tparam ValueT Value type.
template <typename ValueT>
using CUDAScalarView = ScalarView<ValueT>;

/// @brief Non-owning CUDA vector view.
/// @tparam ValueT Value type.
template <typename ValueT>
using CUDAVectorView = VectorView<uint32_t, ValueT>;

/// @brief Functor that XOR-propagates sensitivities along graph edges.
struct CUDASensitivityGenerator {
  /// @brief Sensitivity matrix (`node` x `word`).
  CUDAMatrixView<uint64_t> sensitivities;

  /// @brief Propagate XOR of neighbor sensitivities into @p node at @p word.
  /// @param node Node whose sensitivity word is written.
  /// @param word Sensitivity word index.
  /// @param neighbors Packed neighbor pair (decode with getLower/getUpper).
  DEVICE FORCE_INLINE void
  operator()(uint32_t node, uint32_t word, uint64_t neighbors) const {
    const uint32_t n0 = getLower(neighbors);
    const uint32_t n1 = getUpper(neighbors);

    // Load both neighbor sensitivities (L2-hot in a backwards traversal)
    uint64_t s0 = n0 != UINT32_MAX ? __ldg(&sensitivities(n0, word)) : 0;
    uint64_t s1 = n1 != UINT32_MAX ? __ldg(&sensitivities(n1, word)) : 0;

    uint64_t sum = s0 ^ s1;

    // Direct write (no read) for lower latency and bandwidth
    if (sum != 0) {
      sensitivities(node, word) = sum;
    }
  }
};

/// @brief Functor that builds a packed error class from a node's sensitivities.
/// @tparam W Maximum number of set-bit indices per class.
template <size_t W = 32>
struct CUDAErrorClassGenerator {
  /// @brief Sensitivity matrix (`node` x `word`).
  CUDAMatrixView<uint64_t> sensitivities;

  /// @brief Packed set-bit indices per node (`node` x `W`).
  CUDAMatrixView<uint32_t> classes;

  /// @brief Per-node class hashes.
  CUDAVectorView<uint64_t> hashes;

  /// @brief Per-node identity indices.
  CUDAVectorView<uint32_t> indices;

  /// @brief Number of 64-bit sensitivity words per node.
  uint32_t numWordsPerNode;

  /// @brief Extract set-bit indices of @p node's sensitivities, padded to `W`.
  /// @param node Node whose sensitivity bits are packed.
  /// @return Packed class indices; unused slots are `UINT32_MAX`.
  DEVICE auto getClass(uint32_t node) const -> Array<uint32_t, W> {
    Array<uint32_t, W> cls;
    uint32_t i = 0;

    // Get indices of set bits
    for (uint32_t word = 0; word < numWordsPerNode; ++word) {
      uint64_t value = __ldg(&sensitivities(node, word));

      while (value != 0 && i < W) {
        uint32_t bit = __ffsll(value) - 1;
        cls[i++] = word * 64 + bit;
        value &= (value - 1);
      }
    }

    // Pad with dummies
    for (; i < W; ++i) {
      cls[i] = UINT32_MAX;
    }

    return cls;
  }

  /// @brief Write class, hash, and index for @p node.
  /// @param node Node to materialize.
  DEVICE FORCE_INLINE void operator()(uint32_t node) const {
    auto cls = getClass(node);

    for (uint32_t i = 0; i < W; ++i) {
      classes(node, i) = cls[i];
    }

    hashes[node] = cls.hash64();

    indices[node] = node;
  }
};

/// @brief CUB reduce-by-key op that XOR-merges class probabilities.
struct CUDAErrorClassReducer {
  /// @brief Zipped `(index, probability)` payload.
  using T = thrust::tuple<uint32_t, double>;

  /// @brief XOR-merge probabilities; keep the first index.
  /// @param a First `(index, probability)` pair.
  /// @param b Second `(index, probability)` pair.
  /// @return `(get<0>(a), p0 XOR p1)`.
  DEVICE FORCE_INLINE auto operator()(const T &a, const T &b) const -> T {
    auto p0 = thrust::get<1>(a);
    auto p1 = thrust::get<1>(b);
    auto p = (1 - p1) * p0 + (1 - p0) * p1;
    return thrust::make_tuple(thrust::get<0>(a), p);
  }
};

} // namespace gp

#endif // GREENPEAS_POLICIES_COMPUTE_CUDAHANDLERS_HPP

#ifndef GREENPEAS_QEC_ERRORANALYSIS_WORKSPACE_HPP
#define GREENPEAS_QEC_ERRORANALYSIS_WORKSPACE_HPP

/// Project headers
#include "GreenPeas/Core/DoubleBuffer.hpp"
#include "GreenPeas/Core/Matrix.hpp"
#include "GreenPeas/Core/Scalar.hpp"
#include "GreenPeas/Core/Vector.hpp"
#include "GreenPeas/Policies/Storage/Host.hpp"
#include "GreenPeas/QEC/ErrorAnalysis/Circuit.hpp"

namespace gp {

/// @brief CUB / compute scratch buffer.
/// @tparam Storage Storage policy for allocation/copy.
template <typename Storage>
using Scratchpad = Vector<uint32_t, uint8_t, Storage>;

/// @brief Non-owning view of a compute scratch buffer.
using ScratchpadView = VectorView<uint32_t, uint8_t>;

/// @brief Non-owning view of sensitivity map and dense matrix buffers.
/// @tparam Layout Layout policy.
template <typename Layout>
struct SensitivityWorkspaceView {
  /// @brief Compressed sensitivity map view.
  CircuitSensitivityMapView<Layout> map;

  /// @brief Dense sensitivity matrix view.
  CircuitSensitivityMatrixView<Layout> matrix;
};

/// @brief Owning sensitivity map and dense matrix buffers.
/// @tparam Storage Storage policy for allocation/copy.
/// @tparam Layout Layout policy.
template <typename Storage, typename Layout>
struct SensitivityWorkspace {
  /// @brief Compressed sensitivity map.
  CircuitSensitivityMap<Storage, Layout> map;

  /// @brief Dense sensitivity matrix.
  CircuitSensitivityMatrix<Storage, Layout> matrix;

  /// @brief Construct from unpacked sizes.
  /// @param numNodes Number of error nodes (matrix rows).
  /// @param numMeasurements Number of measurements (map rows).
  /// @param numWordsPerNode Number of 64-bit sensitivity words per node.
  HOST SensitivityWorkspace(uint32_t numNodes,
                            uint32_t numMeasurements,
                            uint32_t numWordsPerNode)
      : map(numMeasurements, numWordsPerNode),
        matrix(numNodes, numWordsPerNode) {}

  /// @brief Fit new unpacked sizes.
  /// @param numNodes New number of error nodes (matrix rows).
  /// @param numMeasurements New number of measurements (map rows).
  /// @param numWordsPerNode New number of 64-bit sensitivity words per node.
  HOST void
  fitto(uint32_t numNodes, uint32_t numMeasurements, uint32_t numWordsPerNode) {
    map.fitto(numMeasurements, numWordsPerNode);
    matrix.fitto(numNodes, numWordsPerNode);
  }

  /// @brief Clear map and matrix contents.
  HOST void clear() {
    map.clear();
    matrix.clear();
  }

  /// @brief Get a non-owning view of this workspace.
  /// @return SensitivityWorkspaceView sharing this workspace's storage.
  HOST auto getView() -> SensitivityWorkspaceView<Layout> {
    return {map.getView(), matrix.getView()};
  }
};

/// @brief Non-owning view of error-class hashing and reduction buffers.
/// @tparam Layout Layout policy for class matrices.
template <typename Layout>
struct ErrorWorkspaceView {
  /// @brief Double-buffered per-node class hashes.
  DoubleBufferView<VectorView<uint32_t, uint64_t>> hashes;

  /// @brief Double-buffered per-node identity / permute indices.
  DoubleBufferView<VectorView<uint32_t, uint32_t>> indices;

  /// @brief Double-buffered packed error-class matrices.
  DoubleBufferView<MatrixView<uint32_t, uint32_t, Layout>> classes;

  /// @brief Double-buffered per-node / per-class probabilities.
  DoubleBufferView<VectorView<uint32_t, double>> probabilities;

  /// @brief Number of unique error classes after reduction.
  ScalarView<uint32_t> numClasses;
};

/// @brief Owning error-class hashing and reduction buffers.
/// @tparam Storage Storage policy for allocation/copy.
/// @tparam Layout Layout policy for class matrices.
/// @tparam W Maximum packed class width.
template <typename Storage, typename Layout, size_t W = 32>
struct ErrorWorkspace {
  /// @brief Double-buffered per-node class hashes.
  DoubleBuffer<Vector<uint32_t, uint64_t, Storage>> hashes;

  /// @brief Double-buffered per-node identity / permute indices.
  DoubleBuffer<Vector<uint32_t, uint32_t, Storage>> indices;

  /// @brief Double-buffered packed error-class matrices.
  DoubleBuffer<Matrix<uint32_t, uint32_t, Storage, Layout>> classes;

  /// @brief Double-buffered per-node / per-class probabilities.
  DoubleBuffer<Vector<uint32_t, double, Storage>> probabilities;

  /// @brief Number of unique error classes after reduction.
  Scalar<uint32_t, Storage> numErrorClasses;

  /// @brief Construct buffers sized for @p numNodes error nodes.
  /// @param numNodes Number of error nodes.
  HOST explicit ErrorWorkspace(uint32_t numNodes)
      : hashes(numNodes), indices(numNodes), classes(numNodes, W),
        probabilities(numNodes) {}

  /// @brief Fit buffers to @p numNodes error nodes.
  /// @param numNodes New number of error nodes.
  HOST void fitto(uint32_t numNodes) {
    hashes.fitto(numNodes);
    indices.fitto(numNodes);
    classes.fitto(numNodes, W);
    probabilities.fitto(numNodes);
  }

  /// @brief Clear hashes, indices, classes, probabilities, and class count.
  HOST void clear() {
    hashes.clear();
    indices.clear();
    classes.clear();
    probabilities.clear();
    numErrorClasses.clear();
  }

  /// @brief Get a non-owning view of this workspace.
  /// @return ErrorWorkspaceView sharing this workspace's storage.
  HOST auto getView() -> ErrorWorkspaceView<Layout> {
    return {hashes.getView(),
            indices.getView(),
            classes.getView(),
            probabilities.getView(),
            numErrorClasses.getView()};
  }
};

/// @brief Host-side reduced detector-error hypergraph buffers.
/// @tparam Layout Layout policy for the class matrix.
/// @tparam W Maximum packed class width.
template <typename Layout, size_t W = 32>
struct HypergraphWorkspace {
  /// @brief Packed detector/observable indices per error class.
  Matrix<uint32_t, uint32_t, HostStorage, Layout> classes;

  /// @brief Probability of each error class.
  Vector<uint32_t, double, HostStorage> probabilities;

  /// @brief Number of unique error classes.
  uint32_t numClasses{};

  /// @brief Construct buffers sized for @p numNodes error nodes.
  /// @param numNodes Number of error nodes (capacity).
  HOST explicit HypergraphWorkspace(uint32_t numNodes)
      : classes(numNodes, W), probabilities(numNodes) {}

  /// @brief Fit buffers to @p numNodes error nodes.
  /// @param numNodes New number of error nodes (capacity).
  HOST void fitto(uint32_t numNodes) {
    classes.fitto(numNodes, W);
    probabilities.fitto(numNodes);
  }
};

} // namespace gp

#endif // GREENPEAS_QEC_ERRORANALYSIS_WORKSPACE_HPP

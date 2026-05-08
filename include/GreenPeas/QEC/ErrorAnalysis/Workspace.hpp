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

/// @brief
template <typename Storage>
using Scratchpad = Vector<uint32_t, uint8_t, Storage>;

/// @brief
using ScratchpadView = VectorView<uint32_t, uint8_t>;

/// @brief Non-owning view of the sensitivity buffers.
template <typename Layout>
struct SensitivityWorkspaceView {
  /// @brief
  CircuitSensitivityMapView<Layout> map;

  /// @brief
  CircuitSensitivityMatrixView<Layout> matrix;
};

/// @brief Owning sensitivity buffers.
template <typename Storage, typename Layout>
struct SensitivityWorkspace {
  /// @brief
  CircuitSensitivityMap<Storage, Layout> map;

  /// @brief
  CircuitSensitivityMatrix<Storage, Layout> matrix;

  /// @brief
  HOST SensitivityWorkspace(uint32_t numNodes,
                            uint32_t numMeasurements,
                            uint32_t numWordsPerNode)
      : map(numMeasurements, numWordsPerNode),
        matrix(numNodes, numWordsPerNode) {}

  /// @brief
  HOST void
  fitto(uint32_t numNodes, uint32_t numMeasurements, uint32_t numWordsPerNode) {
    map.fitto(numMeasurements, numWordsPerNode);
    matrix.fitto(numNodes, numWordsPerNode);
  }

  /// @brief
  HOST void clear() {
    map.clear();
    matrix.clear();
  }

  /// @brief
  HOST auto getView() -> SensitivityWorkspaceView<Layout> {
    return {map.getView(), matrix.getView()};
  }
};

/// @brief Non-owning view of the error buffers.
template <typename Layout>
struct ErrorWorkspaceView {
  /// @brief
  DoubleBufferView<VectorView<uint32_t, uint64_t>> hashes;

  /// @brief
  DoubleBufferView<VectorView<uint32_t, uint32_t>> indices;

  /// @brief
  DoubleBufferView<MatrixView<uint32_t, uint32_t, Layout>> classes;

  /// @brief
  DoubleBufferView<VectorView<uint32_t, double>> probabilities;

  /// @brief
  ScalarView<uint32_t> numClasses;
};

/// @brief Owning error buffers.
template <typename Storage, typename Layout, size_t W = 32>
struct ErrorWorkspace {
  /// @brief
  DoubleBuffer<Vector<uint32_t, uint64_t, Storage>> hashes;

  /// @brief
  DoubleBuffer<Vector<uint32_t, uint32_t, Storage>> indices;

  /// @brief
  DoubleBuffer<Matrix<uint32_t, uint32_t, Storage, Layout>> classes;

  /// @brief
  DoubleBuffer<Vector<uint32_t, double, Storage>> probabilities;

  /// @brief
  Scalar<uint32_t, Storage> numErrorClasses;

  /// @brief
  HOST explicit ErrorWorkspace(uint32_t numNodes)
      : hashes(numNodes), indices(numNodes), classes(numNodes, W),
        probabilities(numNodes) {}

  /// @brief
  HOST void fitto(uint32_t numNodes) {
    hashes.fitto(numNodes);
    indices.fitto(numNodes);
    classes.fitto(numNodes, W);
    probabilities.fitto(numNodes);
  }

  /// @brief
  HOST void clear() {
    hashes.clear();
    indices.clear();
    classes.clear();
    probabilities.clear();
    numErrorClasses.clear();
  }

  /// @brief
  HOST auto getView() -> ErrorWorkspaceView<Layout> {
    return {hashes.getView(),
            indices.getView(),
            classes.getView(),
            probabilities.getView(),
            numErrorClasses.getView()};
  }
};

/// @brief Host-side decoding hypergraph output buffers.
template <typename Layout, size_t W = 32>
struct HypergraphWorkspace {
  /// @brief
  Matrix<uint32_t, uint32_t, HostStorage, Layout> classes;

  /// @brief
  Vector<uint32_t, double, HostStorage> probabilities;

  /// @brief Number of unique error classes after reduction.
  uint32_t numClasses{};

  /// @brief
  HOST explicit HypergraphWorkspace(uint32_t numNodes)
      : classes(numNodes, W), probabilities(numNodes) {}

  /// @brief
  HOST void fitto(uint32_t numNodes) {
    classes.fitto(numNodes, W);
    probabilities.fitto(numNodes);
  }
};

} // namespace gp

#endif // GREENPEAS_QEC_ERRORANALYSIS_WORKSPACE_HPP

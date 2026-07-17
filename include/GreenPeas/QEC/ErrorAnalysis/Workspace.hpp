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

template <typename Storage>
using Scratchpad = Vector<uint32_t, uint8_t, Storage>;

using ScratchpadView = VectorView<uint32_t, uint8_t>;

template <typename Layout>
struct SensitivityWorkspaceView {
  CircuitSensitivityMapView<Layout> map;

  CircuitSensitivityMatrixView<Layout> matrix;
};

template <typename Storage, typename Layout>
struct SensitivityWorkspace {
  CircuitSensitivityMap<Storage, Layout> map;

  CircuitSensitivityMatrix<Storage, Layout> matrix;

  HOST SensitivityWorkspace(uint32_t numNodes,
                            uint32_t numMeasurements,
                            uint32_t numWordsPerNode)
      : map(numMeasurements, numWordsPerNode),
        matrix(numNodes, numWordsPerNode) {}

  HOST void
  fitto(uint32_t numNodes, uint32_t numMeasurements, uint32_t numWordsPerNode) {
    map.fitto(numMeasurements, numWordsPerNode);
    matrix.fitto(numNodes, numWordsPerNode);
  }

  HOST void clear() {
    map.clear();
    matrix.clear();
  }

  HOST auto getView() -> SensitivityWorkspaceView<Layout> {
    return {map.getView(), matrix.getView()};
  }
};

template <typename Layout>
struct ErrorWorkspaceView {
  DoubleBufferView<VectorView<uint32_t, uint64_t>> hashes;

  DoubleBufferView<VectorView<uint32_t, uint32_t>> indices;

  DoubleBufferView<MatrixView<uint32_t, uint32_t, Layout>> classes;

  DoubleBufferView<VectorView<uint32_t, double>> probabilities;

  ScalarView<uint32_t> numClasses;
};

template <typename Storage, typename Layout, size_t W = 32>
struct ErrorWorkspace {
  DoubleBuffer<Vector<uint32_t, uint64_t, Storage>> hashes;

  DoubleBuffer<Vector<uint32_t, uint32_t, Storage>> indices;

  DoubleBuffer<Matrix<uint32_t, uint32_t, Storage, Layout>> classes;

  DoubleBuffer<Vector<uint32_t, double, Storage>> probabilities;

  Scalar<uint32_t, Storage> numErrorClasses;

  HOST explicit ErrorWorkspace(uint32_t numNodes)
      : hashes(numNodes), indices(numNodes), classes(numNodes, W),
        probabilities(numNodes) {}

  HOST void fitto(uint32_t numNodes) {
    hashes.fitto(numNodes);
    indices.fitto(numNodes);
    classes.fitto(numNodes, W);
    probabilities.fitto(numNodes);
  }

  HOST void clear() {
    hashes.clear();
    indices.clear();
    classes.clear();
    probabilities.clear();
    numErrorClasses.clear();
  }

  HOST auto getView() -> ErrorWorkspaceView<Layout> {
    return {hashes.getView(),
            indices.getView(),
            classes.getView(),
            probabilities.getView(),
            numErrorClasses.getView()};
  }
};

template <typename Layout, size_t W = 32>
struct HypergraphWorkspace {
  Matrix<uint32_t, uint32_t, HostStorage, Layout> classes;

  Vector<uint32_t, double, HostStorage> probabilities;

  uint32_t numClasses{};

  HOST explicit HypergraphWorkspace(uint32_t numNodes)
      : classes(numNodes, W), probabilities(numNodes) {}

  HOST void fitto(uint32_t numNodes) {
    classes.fitto(numNodes, W);
    probabilities.fitto(numNodes);
  }
};

} // namespace gp

#endif // GREENPEAS_QEC_ERRORANALYSIS_WORKSPACE_HPP

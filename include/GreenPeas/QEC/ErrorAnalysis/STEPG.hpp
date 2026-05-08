#ifndef GREENPEAS_QEC_ERRORANALYSIS_STEPG_HPP
#define GREENPEAS_QEC_ERRORANALYSIS_STEPG_HPP

/// Standard headers
#include <stdexcept>
#include <string>

/// Project headers
#include "GreenPeas/Common.hpp"
#include "GreenPeas/Core/Graph.hpp"
#include "GreenPeas/Core/Vector.hpp"
#include "GreenPeas/Policies/Storage/Host.hpp"

namespace gp {

/// @brief Space-time (s-t) coordinate.
struct STCoord {
  /// @brief Space index within a time layer.
  uint32_t s;

  /// @brief Time layer index.
  uint32_t t;

  /// @brief Get the row-major linear index of the coordinate.
  /// @param numNodesPerLayer Number of error nodes per layer.
  /// @return `t * numNodesPerLayer + s`.
  HOST auto getIndex(uint32_t numNodesPerLayer) const {
    return t * numNodesPerLayer + s;
  }
};

/// @brief Space-time error propagation graph (STEPG).
///
/// The STEPG represents a syndrome measurement circuit as a directed-acyclic
/// graph (DAG), where nodes denote potential Pauli error locations and edges
/// correspond to gate-induced transformations ("flows") through time.
///
/// It is a wrapper around a Graph<HostStorage> with semantics for adding or
/// removing flows between pairs of source-target nodes given by s-t coords.
struct STEPG {
  /// @brief Number of time layers.
  uint32_t numLayers;

  /// @brief Number of error nodes per layer.
  uint32_t numNodesPerLayer;

  /// @brief Number of error nodes.
  uint32_t numNodes;

  /// @brief Backing binary graph.
  Graph<HostStorage> graph;

  /// @brief Error node probabilities.
  Vector<uint32_t, double, HostStorage> probs;

  /// @brief Construct a STEPG from:
  /// @param numLayers The number of time layers.
  /// @param numNodesPerLayer The number of error nodes per layer.
  HOST STEPG(uint32_t numLayers, uint32_t numNodesPerLayer)
      : numLayers(numLayers), numNodesPerLayer(numNodesPerLayer),
        numNodes(numLayers * numNodesPerLayer), graph(numNodes),
        probs(numNodes) {}

  /// @brief Fit new number of time layers and error nodes per layer.
  /// @param newNumLayers New number of time layers.
  /// @param newNumNodesPerLayer New number of error nodes per layer.
  HOST void fitto(uint32_t newNumLayers, uint32_t newNumNodesPerLayer) {
    numLayers = newNumLayers;
    numNodesPerLayer = newNumNodesPerLayer;
    numNodes = newNumLayers * newNumNodesPerLayer;
    graph.fitto(numNodes);
    probs.fitto(numNodes);
  }

  /// @brief Add a flow (directed edge) from a source to a target node.
  /// @param sourceCoord Source s-t coordinate.
  /// @param targetCoord Target s-t coordinate.
  /// @throws std::runtime_error If the flow does not move forward in time or if
  /// the update to the binary graph fails.
  HOST void addFlow(STCoord sourceCoord, STCoord targetCoord) {
    if (targetCoord.t <= sourceCoord.t) {
      throw std::runtime_error("STEPG: flows must move forward in time.");
    }

    const auto source = sourceCoord.getIndex(numNodesPerLayer);
    const auto target = targetCoord.getIndex(numNodesPerLayer);

    const auto status = graph.addEdge(source, target);

    if (status != UpdateStatus::ok) {
      throw std::runtime_error(
          std::string("STEPG: graph.addEdge failed with status ") +
          std::to_string(static_cast<int>(status)) + ".");
    }
  }

  /// @brief Remove a flow (directed edge) from a source to a target node.
  /// @param sourceCoord Source s-t coordinate.
  /// @param targetCoord Target s-t coordinate.
  /// @throws std::runtime_error If the flow does not move forward in time or if
  /// the update to the binary graph fails.
  HOST void removeFlow(STCoord sourceCoord, STCoord targetCoord) {
    if (targetCoord.t <= sourceCoord.t) {
      throw std::runtime_error("STEPG: flows must move forward in time.");
    }

    const auto source = sourceCoord.getIndex(numNodesPerLayer);
    const auto target = targetCoord.getIndex(numNodesPerLayer);

    const auto status = graph.removeEdge(source, target);

    if (status != UpdateStatus::ok) {
      throw std::runtime_error(
          std::string("STEPG: graph.removeEdge failed with status ") +
          std::to_string(static_cast<int>(status)) + ".");
    }
  }

  /// @brief XOR-merge the stored probability at @p coord with @p pNew.
  /// @param coord s-t coordinate of the error node.
  /// @param pNew New probability to XOR-merge with the old value.
  HOST void mergeProbabilities(STCoord coord, double pNew) {
    auto &pOld = probs[coord.getIndex(numNodesPerLayer)];
    pOld = (1 - pNew) * pOld + (1 - pOld) * pNew;
  }
};

} // namespace gp

#endif // GREENPEAS_QEC_ERRORANALYSIS_STEPG_HPP

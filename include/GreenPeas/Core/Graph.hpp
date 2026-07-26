#ifndef GREENPEAS_CORE_GRAPH_HPP
#define GREENPEAS_CORE_GRAPH_HPP

/// Standard headers
#include <stdexcept>

/// Project headers
#include "GreenPeas/Common.hpp"
#include "GreenPeas/Core/Vector.hpp"
#include "GreenPeas/Core/Words.hpp"

namespace gp {

/// @brief Result of adding/removing an edge (u, v) to/from a graph.
enum class UpdateStatus : uint8_t {
  /// Operation succeeded
  ok,
  /// Edge (u, v) already exists
  already_exists,
  /// Edge (u, v) not found
  not_found,
  /// Node u already has 2 outedges
  full,
  /// u or v are out of bounds (e.g. >= numNodes)
  out_of_bounds
};

/// @brief Non-owning view over a graph.
struct GraphView {
  /// @brief Number of nodes in the graph.
  uint32_t numNodes;

  /// @brief Non-owning view of the adjacency list.
  VectorView<uint32_t, uint64_t> adjacencies;

  /// @brief Get the packed neighbors of a node (decode with getLower/getUpper).
  /// @param node The index of the node.
  /// @return Reference to the packed neighbors.
  HOST DEVICE FORCE_INLINE auto operator[](uint32_t node) -> uint64_t & {
    return adjacencies[node];
  }

  /// @brief Get the packed neighbors of a node (decode with getLower/getUpper).
  /// @param node The index of the node.
  /// @return Reference to the packed neighbors.
  HOST DEVICE FORCE_INLINE auto operator[](uint32_t node) const -> uint64_t & {
    return adjacencies[node];
  }
};

/// @brief Binary (max degree 2) directed graph.
///
/// @tparam Storage Storage policy for heap-allocated memory.
template <typename Storage>
struct Graph {
  /// @brief Maximum number of nodes. Invariant: numNodes <= maxNumNodes.
  uint32_t maxNumNodes;

  /// @brief Current number of nodes. Must be <= maxNumNodes.
  uint32_t numNodes;

  /// @brief The adjacency list.
  Vector<uint32_t, uint64_t, Storage> adjacencies;

  /// @brief Constructs a graph with given number of nodes.
  /// Initially, all nodes have zero degree, so
  /// - getUpper(adjacencies[node]) == UINT32_MAX (sentinel)
  /// - getLower(adjacencies[node]) == UINT32_MAX (sentinel)
  /// @param numNodes The number of nodes in the graph.
  HOST explicit Graph(uint32_t numNodes)
      : maxNumNodes(numNodes), numNodes(numNodes), adjacencies(numNodes) {
    adjacencies.set();
  };

  /// @brief Get the packed neighbors of a node (decode with getLower/getUpper).
  /// @param node The index of the node.
  /// @return Reference to the packed neighbors.
  HOST auto operator[](uint32_t node) -> uint64_t & {
    return adjacencies[node];
  }

  /// @brief Get the packed neighbors of a node (decode with getLower/getUpper).
  /// @param node The index of the node.
  /// @return Reference to the packed neighbors.
  HOST auto operator[](uint32_t node) const -> uint64_t {
    return adjacencies[node];
  }

  /// @brief Fit new number of nodes (must be <= maxNumNodes).
  /// @param newNumNodes New number of nodes.
  HOST void fitto(uint32_t newNumNodes) {
    if (newNumNodes > maxNumNodes) {
      throw std::runtime_error("Graph: newSize exceeds maxNumNodes.");
    }
    numNodes = newNumNodes;
    adjacencies.fitto(newNumNodes);
  }

  /// @brief Copy to another graph.
  /// @tparam DestinationStorage Storage policy of the destination graph.
  /// @param other Destination graph (must have same numNodes).
  template <typename DestinationStorage>
  HOST void copyTo(Graph<DestinationStorage> &other) const {
    if (numNodes != other.numNodes) {
      throw std::runtime_error("Graph: numNodes mismatch in copyTo.");
    }
    adjacencies.copyTo(other.adjacencies);
  }

  /// @brief Copy from another graph.
  /// @tparam SourceStorage Storage policy of the source graph.
  /// @param other Source graph (must have same numNodes).
  template <typename SourceStorage>
  HOST void copyFrom(const Graph<SourceStorage> &other) {
    if (numNodes != other.numNodes) {
      throw std::runtime_error("Graph: numNodes mismatch in copyFrom.");
    }
    adjacencies.copyFrom(other.adjacencies);
  }

  /// @brief Reset the graph by setting all adjacency entries to the sentinel.
  HOST void reset() { adjacencies.set(); }

  /// @brief Get a non-owning view of the graph.
  /// @return GraphView The non-owning view.
  HOST auto getView() -> GraphView { return {numNodes, adjacencies.getView()}; }

  /// @brief Add a directed edge from source to target.
  /// @param source The index of the source node.
  /// @param target The index of the target node.
  /// @return UpdateStatus (ok, already_exists, full, or out_of_bounds).
  HOST auto addEdge(uint32_t source, uint32_t target) -> UpdateStatus {
    if (source >= numNodes || target >= numNodes) {
      return UpdateStatus::out_of_bounds;
    }

    auto &targets = (*this)[source];
    const auto target0 = getLower(targets);
    const auto target1 = getUpper(targets);

    if (target0 == target || target1 == target) {
      return UpdateStatus::already_exists;
    }

    if (target0 == UINT32_MAX) {
      setLower(targets, target);
      return UpdateStatus::ok;
    }

    if (target1 == UINT32_MAX) {
      setUpper(targets, target);
      return UpdateStatus::ok;
    }

    return UpdateStatus::full;
  }

  /// @brief Remove a directed edge from source to target.
  /// @param source The index of the source node.
  /// @param target The index of the target node.
  /// @return UpdateStatus (ok, not_found, or out_of_bounds).
  HOST auto removeEdge(uint32_t source, uint32_t target) -> UpdateStatus {
    if (source >= numNodes || target >= numNodes) {
      return UpdateStatus::out_of_bounds;
    }

    auto &targets = (*this)[source];
    const auto target0 = getLower(targets);
    const auto target1 = getUpper(targets);

    if (target0 == target) {
      setLower(targets, UINT32_MAX);
      return UpdateStatus::ok;
    }

    if (target1 == target) {
      setUpper(targets, UINT32_MAX);
      return UpdateStatus::ok;
    }

    return UpdateStatus::not_found;
  }
};

} // namespace gp

#endif // GREENPEAS_CORE_GRAPH_HPP

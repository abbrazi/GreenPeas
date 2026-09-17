#ifndef GREENPEAS_QEC_ERRORANALYSIS_STEPG_HPP
#define GREENPEAS_QEC_ERRORANALYSIS_STEPG_HPP

/// Standard headers
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

/// Project headers
#include "GreenPeas/Common.hpp"
#include "GreenPeas/Core/Graph.hpp"
#include "GreenPeas/Core/Vector.hpp"
#include "GreenPeas/Core/Words.hpp"
#include "GreenPeas/Policies/Storage/Host.hpp"

/// Third-party headers
#include "pugixml.hpp"

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

/// @brief Serialize a pugixml document to a UTF-8 string.
/// @param doc XML document to serialize.
/// @return Pretty-printed XML as a UTF-8 string.
HOST inline auto toString(const pugi::xml_document &doc) -> std::string {
  std::ostringstream stream;
  doc.save(stream, "  ");
  return stream.str();
}

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

  /// @brief Reset the graph and clear the error node probabilities.
  HOST void reset() {
    graph.reset();
    probs.clear();
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

  /// @brief Serialize the STEPG to GraphML.
  /// @return GraphML XML document as a UTF-8 string.
  HOST auto getXML() const -> std::string {
    pugi::xml_document doc;

    auto declaration = doc.append_child(pugi::node_declaration);
    declaration.append_attribute("version") = "1.0";
    declaration.append_attribute("encoding") = "UTF-8";

    auto graphml = doc.append_child("graphml");
    graphml.append_attribute("xmlns") = "http://graphml.graphdrawing.org/xmlns";

    auto appendKey = [&](const char *id,
                         const char *forWhat,
                         const char *name,
                         const char *type) {
      auto key = graphml.append_child("key");
      key.append_attribute("id") = id;
      key.append_attribute("for") = forWhat;
      key.append_attribute("attr.name") = name;
      key.append_attribute("attr.type") = type;
    };

    appendKey("d0", "node", "x", "long");
    appendKey("d1", "node", "y", "long");

    auto graphNode = graphml.append_child("graph");
    graphNode.append_attribute("id") = "G";
    graphNode.append_attribute("edgedefault") = "directed";

    auto appendData = [](pugi::xml_node parent, const char *key, auto value) {
      auto data = parent.append_child("data");
      data.append_attribute("key") = key;
      data.text().set(value);
    };

    // Flag non-zero-degree nodes.
    std::vector<bool> connected(numNodes, false);
    for (uint32_t source = 0; source < numNodes; ++source) {
      const auto targets = graph[source];
      const auto target0 = getLower(targets);
      const auto target1 = getUpper(targets);

      if (target0 != UINT32_MAX) {
        connected[source] = connected[target0] = true;
      }

      if (target1 != UINT32_MAX) {
        connected[source] = connected[target1] = true;
      }
    }

    for (uint32_t i = 0; i < numNodes; ++i) {
      if (!connected[i]) {
        continue;
      }

      auto node = graphNode.append_child("node");
      node.append_attribute("id") = i;
      appendData(node, "d0", i % numNodesPerLayer);
      appendData(node, "d1", i / numNodesPerLayer);
    }

    for (uint32_t source = 0; source < numNodes; ++source) {
      const auto targets = graph[source];
      const auto target0 = getLower(targets);
      const auto target1 = getUpper(targets);

      if (target0 != UINT32_MAX) {
        auto edge = graphNode.append_child("edge");
        edge.append_attribute("source") = source;
        edge.append_attribute("target") = target0;
      }

      if (target1 != UINT32_MAX) {
        auto edge = graphNode.append_child("edge");
        edge.append_attribute("source") = source;
        edge.append_attribute("target") = target1;
      }
    }

    return toString(doc);
  }
};

} // namespace gp

#endif // GREENPEAS_QEC_ERRORANALYSIS_STEPG_HPP

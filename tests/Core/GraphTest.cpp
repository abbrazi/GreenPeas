/// Standard headers
#include <cstdint>
#include <stdexcept>
#include <utility>

/// Helper headers
#include "../Helpers/Macros.hpp"

/// Project headers
#include "GreenPeas/Core/Graph.hpp"
#include "GreenPeas/Policies/Storage/Host.hpp"

using namespace gp;

using HostGraph = Graph<HostStorage>;

// --- Graph ---

static void testGraphExplicitConstructor() {
  constexpr uint32_t n = 20;
  HostGraph graph(n);
  REQUIRE(graph.numNodes == n);
  REQUIRE(graph.maxNumNodes == n);
  REQUIRE(graph.adjacencies.size == n);
  REQUIRE(graph.adjacencies.data != nullptr);
}

static void testGraphGetElement() {
  HostGraph graph(5);
  constexpr uint32_t node = 2;
  graph[node] = 0;
  setLower(graph[node], 10);
  REQUIRE(getLower(graph[node]) == 10);
}

static void testGraphSetElement() {
  HostGraph graph(5);
  constexpr uint32_t node = 2;
  setUpper(graph[node], 10);
  REQUIRE(getUpper(graph[node]) == 10);
}

static void testGraphConstElementAccess() {
  HostGraph graph(3);
  setLower(graph[1], 99);
  const HostGraph &constGraph = graph;
  REQUIRE(getLower(constGraph[1]) == 99);
}

static void testGraphFittoOk() {
  HostGraph graph(4);
  REQUIRE(graph.numNodes == 4);
  REQUIRE(graph.maxNumNodes == 4);
  REQUIRE(graph.adjacencies.size == 4);
  graph.fitto(2);
  REQUIRE(graph.numNodes == 2);
  REQUIRE(graph.maxNumNodes == 4);
  REQUIRE(graph.adjacencies.size == 2);
}

static void testGraphFittoThrowsWhenExceedsMax() {
  HostGraph graph(4);
  bool threw = false;
  try {
    graph.fitto(5);
  } catch (const std::runtime_error &) {
    threw = true;
  }
  REQUIRE(threw);
  REQUIRE(graph.numNodes == 4);
}

static void testGraphCopyToHost() {
  HostGraph graph1(3);
  setLower(graph1[0], 1);
  setUpper(graph1[0], 2);
  setLower(graph1[2], 3);
  setUpper(graph1[2], 4);
  HostGraph graph2(3);
  graph1.copyTo(graph2);
  REQUIRE(getLower(graph2[0]) == 1);
  REQUIRE(getUpper(graph2[0]) == 2);
  REQUIRE(getLower(graph2[2]) == 3);
  REQUIRE(getUpper(graph2[2]) == 4);
}

static void testGraphCopyToThrowsOnNumNodesMismatch() {
  HostGraph graph1(2);
  HostGraph graph2(3);
  bool threw = false;
  try {
    graph1.copyTo(graph2);
  } catch (const std::runtime_error &) {
    threw = true;
  }
  REQUIRE(threw);
}

static void testGraphCopyFromHost() {
  HostGraph graph1(3);
  setLower(graph1[0], 1);
  setUpper(graph1[0], 2);
  setLower(graph1[2], 3);
  setUpper(graph1[2], 4);
  HostGraph graph2(3);
  graph2.copyFrom(graph1);
  REQUIRE(getLower(graph2[0]) == 1);
  REQUIRE(getUpper(graph2[0]) == 2);
  REQUIRE(getLower(graph2[2]) == 3);
  REQUIRE(getUpper(graph2[2]) == 4);
}

static void testGraphCopyFromThrowsOnNumNodesMismatch() {
  HostGraph graph1(2);
  HostGraph graph2(3);
  bool threw = false;
  try {
    graph1.copyFrom(graph2);
  } catch (const std::runtime_error &) {
    threw = true;
  }
  REQUIRE(threw);
}

static void testGraphGetView() {
  HostGraph graph(2);
  setLower(graph[0], 42);
  auto view = graph.getView();
  REQUIRE(view.numNodes == graph.numNodes);
  REQUIRE(view.adjacencies.data == graph.adjacencies.data);
  REQUIRE(view.adjacencies.size == graph.adjacencies.size);
  REQUIRE(getLower(std::as_const(view)[0]) == 42);
  setUpper(view[1], 100);
  REQUIRE(getUpper(graph[1]) == 100);
  REQUIRE(getUpper(std::as_const(view)[1]) == 100);
}

static void testGraphAddEdgeSourceOutOfBounds() {
  HostGraph graph(20);
  const uint32_t source = graph.numNodes;
  const uint32_t target = 0;

  const auto status = graph.addEdge(source, target);

  REQUIRE(status == UpdateStatus::out_of_bounds);
}

static void testGraphAddEdgeTargetOutOfBounds() {
  HostGraph graph(20);
  const uint32_t source = 0;
  const uint32_t target = graph.numNodes;

  const auto status = graph.addEdge(source, target);

  REQUIRE(status == UpdateStatus::out_of_bounds);
}

static void testGraphAddEdgeTargetAlreadyInLowerField() {
  HostGraph graph(20);
  const uint32_t source = 0;
  const uint32_t target = graph.numNodes / 2;

  setLower(graph[source], target);

  const auto status = graph.addEdge(source, target);

  REQUIRE(status == UpdateStatus::already_exists);
}

static void testGraphAddEdgeTargetAlreadyInUpperField() {
  HostGraph graph(20);
  const uint32_t source = 0;
  const uint32_t target = graph.numNodes / 2 + 2;

  setUpper(graph[source], target);

  const auto status = graph.addEdge(source, target);

  REQUIRE(status == UpdateStatus::already_exists);
}

static void testGraphAddEdgeTargetInLowerField() {
  HostGraph graph(20);
  const uint32_t source = 0;
  const uint32_t target = graph.numNodes / 2;

  const auto status = graph.addEdge(source, target);

  REQUIRE(status == UpdateStatus::ok);
  REQUIRE(getLower(graph[source]) == target);
}

static void testGraphAddEdgeTargetInUpperField() {
  HostGraph graph(20);
  const uint32_t source = 0;
  const uint32_t target = graph.numNodes / 2 + 2;

  setLower(graph[source], target - 2);

  const auto status = graph.addEdge(source, target);

  REQUIRE(status == UpdateStatus::ok);
  REQUIRE(getUpper(graph[source]) == target);
}

static void testGraphAddEdgeNoEmptySlots() {
  HostGraph graph(20);
  const uint32_t source = 0;
  const uint32_t target = graph.numNodes / 2;

  setLower(graph[source], target + 1);
  setUpper(graph[source], target + 2);

  const auto status = graph.addEdge(source, target);

  REQUIRE(status == UpdateStatus::full);
}

static void testGraphRemoveEdgeSourceOutOfBounds() {
  HostGraph graph(20);
  const uint32_t source = graph.numNodes;
  const uint32_t target = 0;

  const auto status = graph.removeEdge(source, target);

  REQUIRE(status == UpdateStatus::out_of_bounds);
}

static void testGraphRemoveEdgeTargetOutOfBounds() {
  HostGraph graph(20);
  const uint32_t source = 0;
  const uint32_t target = graph.numNodes;

  const auto status = graph.removeEdge(source, target);

  REQUIRE(status == UpdateStatus::out_of_bounds);
}

static void testGraphRemoveEdgeTargetInLowerField() {
  HostGraph graph(20);
  const uint32_t source = 0;
  const uint32_t target = graph.numNodes / 2;

  graph.addEdge(source, target);

  const auto status = graph.removeEdge(source, target);

  REQUIRE(status == UpdateStatus::ok);
  REQUIRE(getLower(graph[source]) == UINT32_MAX);
}

static void testGraphRemoveEdgeTargetInUpperField() {
  HostGraph graph(20);
  const uint32_t source = 0;
  const uint32_t target = graph.numNodes / 2 + 2;

  graph.addEdge(source, target - 2);
  graph.addEdge(source, target);

  const auto status = graph.removeEdge(source, target);

  REQUIRE(status == UpdateStatus::ok);
  REQUIRE(getUpper(graph[source]) == UINT32_MAX);
}

static void testGraphRemoveEdgeNotFound() {
  HostGraph graph(20);
  const uint32_t source = 0;
  const uint32_t target = graph.numNodes / 2;

  graph.addEdge(source, target + 1);
  graph.addEdge(source, target + 2);

  const auto status = graph.removeEdge(source, target);

  REQUIRE(status == UpdateStatus::not_found);
}

auto main() -> int {
  // --- Graph ---

  // Constructor
  testGraphExplicitConstructor();

  // Graph::operator[] (non-const)
  testGraphGetElement();
  testGraphSetElement();

  // Graph::operator[] (const)
  testGraphConstElementAccess();

  // Graph::fitto
  testGraphFittoOk();
  testGraphFittoThrowsWhenExceedsMax();

  // Graph::copyTo
  testGraphCopyToHost();
  testGraphCopyToThrowsOnNumNodesMismatch();

  // Graph::copyFrom
  testGraphCopyFromHost();
  testGraphCopyFromThrowsOnNumNodesMismatch();

  // Graph::getView
  testGraphGetView();

  // Graph::addEdge
  testGraphAddEdgeSourceOutOfBounds();
  testGraphAddEdgeTargetOutOfBounds();
  testGraphAddEdgeTargetAlreadyInLowerField();
  testGraphAddEdgeTargetAlreadyInUpperField();
  testGraphAddEdgeTargetInLowerField();
  testGraphAddEdgeTargetInUpperField();
  testGraphAddEdgeNoEmptySlots();

  // Graph::removeEdge
  testGraphRemoveEdgeSourceOutOfBounds();
  testGraphRemoveEdgeTargetOutOfBounds();
  testGraphRemoveEdgeTargetInLowerField();
  testGraphRemoveEdgeTargetInUpperField();
  testGraphRemoveEdgeNotFound();

  // All tests passed!
  return 0;
}

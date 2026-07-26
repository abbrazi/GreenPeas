/// Standard headers
#include <cstdint>
#include <stdexcept>

/// Helper headers
#include "../../Helpers/Macros.hpp"

/// Project headers
#include "GreenPeas/Core/Words.hpp"
#include "GreenPeas/QEC/ErrorAnalysis/STEPG.hpp"

using namespace gp;

// --- STCoord ---

static void testSTCoordGetIndex() {
  constexpr uint32_t numNodesPerLayer = 4;
  const STCoord coord{1, 2};
  REQUIRE(coord.getIndex(numNodesPerLayer) == 2 * numNodesPerLayer + 1);
}

// --- STEPG ---

static void testSTEPGConstructor() {
  STEPG stepg(3, 4);
  REQUIRE(stepg.numLayers == 3);
  REQUIRE(stepg.numNodesPerLayer == 4);
  REQUIRE(stepg.numNodes == 12);
  REQUIRE(stepg.graph.numNodes == 12);
  REQUIRE(stepg.graph.maxNumNodes == 12);
}

static void testSTEPGFittoOk() {
  STEPG stepg(4, 4);

  stepg.fitto(2, 3);

  REQUIRE(stepg.numLayers == 2);
  REQUIRE(stepg.numNodesPerLayer == 3);
  REQUIRE(stepg.numNodes == 6);
  REQUIRE(stepg.graph.numNodes == 6);
  REQUIRE(stepg.graph.maxNumNodes == 16);
  REQUIRE(stepg.probs.size == 6);
  REQUIRE(stepg.probs.maxSize == 16);
}

static void testSTEPGResetResetsGraphAndProbs() {
  STEPG stepg(3, 3);

  stepg.addFlow({0, 0}, {0, 1});
  stepg.probs[0] = 0.5;

  stepg.reset();

  REQUIRE(getLower(stepg.graph[0]) == UINT32_MAX);
  REQUIRE(getUpper(stepg.graph[0]) == UINT32_MAX);
  REQUIRE(stepg.probs[0] == 0.0);
}

static void testSTEPGAddFlowUpdatesGraph() {
  STEPG stepg(4, 4);

  // Case: same s
  STCoord sourceCoord{0, 0};
  STCoord targetCoord{0, 1};

  stepg.addFlow(sourceCoord, targetCoord);

  uint32_t source = sourceCoord.getIndex(stepg.numNodesPerLayer);
  uint32_t target = targetCoord.getIndex(stepg.numNodesPerLayer);

  REQUIRE(getLower(stepg.graph[source]) == target);

  // Case: different s
  sourceCoord.s = 2;
  targetCoord.s = 3;

  stepg.addFlow(sourceCoord, targetCoord);

  source = sourceCoord.getIndex(stepg.numNodesPerLayer);
  target = targetCoord.getIndex(stepg.numNodesPerLayer);

  REQUIRE(getLower(stepg.graph[source]) == target);
}

static void testSTEPGAddFlowThrowsWhenAdjacentButBackward() {
  STEPG stepg(2, 2);

  const STCoord sourceCoord{0, 1};
  const STCoord targetCoord{0, 0};

  bool threw = false;
  try {
    stepg.addFlow(sourceCoord, targetCoord);
  } catch (const std::runtime_error &) {
    threw = true;
  }

  REQUIRE(threw);
}

static void testSTEPGAddFlowThrowsWhenSameTimeLayer() {
  STEPG stepg(2, 2);

  const STCoord sourceCoord{0, 1};
  const STCoord targetCoord{1, 1};

  bool threw = false;
  try {
    stepg.addFlow(sourceCoord, targetCoord);
  } catch (const std::runtime_error &) {
    threw = true;
  }

  REQUIRE(threw);
}

static void testSTEPGAddFlowThrowsWhenGraphUpdateFails() {
  STEPG stepg(3, 3);

  const STCoord sourceCoord{0, 0};
  const STCoord targetCoord{1, 1};

  stepg.addFlow(sourceCoord, targetCoord);

  bool threw = false;
  try {
    stepg.addFlow(sourceCoord, targetCoord);
  } catch (const std::runtime_error &) {
    threw = true;
  }

  REQUIRE(threw);
}

static void testSTEPGRemoveFlowUpdatesGraph() {
  STEPG stepg(3, 3);

  const STCoord sourceCoord{0, 0};
  const STCoord targetCoord{1, 1};

  stepg.addFlow(sourceCoord, targetCoord);

  stepg.removeFlow(sourceCoord, targetCoord);

  const uint32_t source = sourceCoord.getIndex(stepg.numNodesPerLayer);

  REQUIRE(getLower(stepg.graph[source]) == UINT32_MAX);
}

static void testSTEPGRemoveFlowThrowsWhenAdjacentButBackward() {
  STEPG stepg(2, 2);

  const STCoord sourceCoord{0, 1};
  const STCoord targetCoord{0, 0};

  bool threw = false;
  try {
    stepg.removeFlow(sourceCoord, targetCoord);
  } catch (const std::runtime_error &) {
    threw = true;
  }

  REQUIRE(threw);
}

static void testSTEPGRemoveFlowThrowsWhenSameTimeLayer() {
  STEPG stepg(2, 2);

  const STCoord sourceCoord{0, 1};
  const STCoord targetCoord{1, 1};

  bool threw = false;
  try {
    stepg.removeFlow(sourceCoord, targetCoord);
  } catch (const std::runtime_error &) {
    threw = true;
  }

  REQUIRE(threw);
}

static void testSTEPGRemoveFlowThrowsWhenGraphUpdateFails() {
  STEPG stepg(3, 3);

  const STCoord sourceCoord{0, 0};
  const STCoord targetCoord{1, 1};

  bool threw = false;
  try {
    stepg.removeFlow(sourceCoord, targetCoord);
  } catch (const std::runtime_error &) {
    threw = true;
  }

  REQUIRE(threw);
}

static void testSTEPGMergeProbabilities() {
  STEPG stepg(2, 4);

  const STCoord coord(1, 0);
  const uint32_t index = coord.getIndex(stepg.numNodesPerLayer);

  stepg.probs[index] = 0.25;

  stepg.mergeProbabilities(coord, 0.75);

  REQUIRE(stepg.probs[index] == 0.625);
}

auto main() -> int {
  // --- STCoord ---

  // STCoord::getIndex
  testSTCoordGetIndex();

  // --- STEPG ---

  // Constructor
  testSTEPGConstructor();

  // STEPG::fitto
  testSTEPGFittoOk();

  // STEPG::reset
  testSTEPGResetResetsGraphAndProbs();

  // STEPG::addFlow
  testSTEPGAddFlowUpdatesGraph();
  testSTEPGAddFlowThrowsWhenAdjacentButBackward();
  testSTEPGAddFlowThrowsWhenSameTimeLayer();
  testSTEPGAddFlowThrowsWhenGraphUpdateFails();

  // STEPG::removeFlow
  testSTEPGRemoveFlowUpdatesGraph();
  testSTEPGRemoveFlowThrowsWhenAdjacentButBackward();
  testSTEPGRemoveFlowThrowsWhenSameTimeLayer();
  testSTEPGRemoveFlowThrowsWhenGraphUpdateFails();

  // STEPG::mergeProbabilities
  testSTEPGMergeProbabilities();

  // All tests passed!
  return 0;
}

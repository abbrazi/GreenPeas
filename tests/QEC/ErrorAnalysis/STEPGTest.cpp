/// Standard headers
#include <cstdint>
#include <stdexcept>
#include <string>

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

static void testSTEPGGetXML() {
  STEPG stepg(3, 5);

  // Persistent flows
  stepg.addFlow({0, 0}, {0, 1});
  stepg.addFlow({1, 0}, {1, 1});
  stepg.addFlow({2, 0}, {2, 1});
  stepg.addFlow({3, 0}, {3, 1});
  stepg.addFlow({4, 0}, {4, 1});
  stepg.addFlow({0, 1}, {0, 2});
  stepg.addFlow({1, 1}, {1, 2});
  stepg.addFlow({2, 1}, {2, 2});
  stepg.addFlow({3, 1}, {3, 2});
  stepg.addFlow({4, 1}, {4, 2});

  // Propagation flows
  stepg.addFlow({2, 0}, {1, 1});
  stepg.addFlow({4, 0}, {3, 1});
  stepg.addFlow({0, 1}, {1, 2});
  stepg.addFlow({2, 1}, {3, 2});

  const std::string expected = R"(<?xml version="1.0" encoding="UTF-8"?>
<graphml xmlns="http://graphml.graphdrawing.org/xmlns">
  <key id="d0" for="node" attr.name="x" attr.type="long" />
  <key id="d1" for="node" attr.name="y" attr.type="long" />
  <graph id="G" edgedefault="directed">
    <node id="0">
      <data key="d0">0</data>
      <data key="d1">0</data>
    </node>
    <node id="1">
      <data key="d0">1</data>
      <data key="d1">0</data>
    </node>
    <node id="2">
      <data key="d0">2</data>
      <data key="d1">0</data>
    </node>
    <node id="3">
      <data key="d0">3</data>
      <data key="d1">0</data>
    </node>
    <node id="4">
      <data key="d0">4</data>
      <data key="d1">0</data>
    </node>
    <node id="5">
      <data key="d0">0</data>
      <data key="d1">1</data>
    </node>
    <node id="6">
      <data key="d0">1</data>
      <data key="d1">1</data>
    </node>
    <node id="7">
      <data key="d0">2</data>
      <data key="d1">1</data>
    </node>
    <node id="8">
      <data key="d0">3</data>
      <data key="d1">1</data>
    </node>
    <node id="9">
      <data key="d0">4</data>
      <data key="d1">1</data>
    </node>
    <node id="10">
      <data key="d0">0</data>
      <data key="d1">2</data>
    </node>
    <node id="11">
      <data key="d0">1</data>
      <data key="d1">2</data>
    </node>
    <node id="12">
      <data key="d0">2</data>
      <data key="d1">2</data>
    </node>
    <node id="13">
      <data key="d0">3</data>
      <data key="d1">2</data>
    </node>
    <node id="14">
      <data key="d0">4</data>
      <data key="d1">2</data>
    </node>
    <edge source="0" target="5" />
    <edge source="1" target="6" />
    <edge source="2" target="7" />
    <edge source="2" target="6" />
    <edge source="3" target="8" />
    <edge source="4" target="9" />
    <edge source="4" target="8" />
    <edge source="5" target="10" />
    <edge source="5" target="11" />
    <edge source="6" target="11" />
    <edge source="7" target="12" />
    <edge source="7" target="13" />
    <edge source="8" target="13" />
    <edge source="9" target="14" />
  </graph>
</graphml>
)";

  REQUIRE(stepg.getXML() == expected);
}

static void testSTEPGGetXMLSkipsZeroDegreeNodes() {
  STEPG stepg(2, 3);

  // Only s = 0 carries a flow, so s = 1 and s = 2 are left out.
  stepg.addFlow({0, 0}, {0, 1});

  const std::string expected = R"(<?xml version="1.0" encoding="UTF-8"?>
<graphml xmlns="http://graphml.graphdrawing.org/xmlns">
  <key id="d0" for="node" attr.name="x" attr.type="long" />
  <key id="d1" for="node" attr.name="y" attr.type="long" />
  <graph id="G" edgedefault="directed">
    <node id="0">
      <data key="d0">0</data>
      <data key="d1">0</data>
    </node>
    <node id="3">
      <data key="d0">0</data>
      <data key="d1">1</data>
    </node>
    <edge source="0" target="3" />
  </graph>
</graphml>
)";

  REQUIRE(stepg.getXML() == expected);
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

  // STEPG::getXML
  testSTEPGGetXML();
  testSTEPGGetXMLSkipsZeroDegreeNodes();

  // All tests passed!
  return 0;
}

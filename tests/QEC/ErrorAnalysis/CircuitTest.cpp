/// Standard headers
#include <vector>

/// Helper headers
#include "../../Helpers/Macros.hpp"

/// Project headers
#include "GreenPeas/Policies/Data/Layout.hpp"
#include "GreenPeas/QEC/ErrorAnalysis/Circuit.hpp"

/// Third-party headers
#include "stim.h"

using namespace gp;

template <typename Layout>
static void testCircuitConstructor() {
  constexpr CircuitParameters<CorrelationLevel::L2> parameters{4, 3, 4, 2, 1};

  const Circuit<Layout, CorrelationLevel::L2> circuit(parameters);

  REQUIRE(circuit.parameters == parameters);
  REQUIRE(circuit.stepg.numLayers == 3);
  REQUIRE(circuit.stepg.numNodesPerLayer == 32);
  REQUIRE(circuit.stepg.numNodes == 96);
  REQUIRE(circuit.sensitivityMap.keys.size == 4);
  REQUIRE(circuit.sensitivityMap.vals.dimensions.numRows == 4);
  REQUIRE(circuit.sensitivityMap.vals.dimensions.numCols == 1);
  REQUIRE(circuit.counters.layer == 0);
  REQUIRE(circuit.counters.measurement == 0);
  REQUIRE(circuit.counters.detector == 0);
}

template <typename Layout>
static void testCircuitApplyOpDetector() {
  constexpr CircuitParameters<CorrelationLevel::L2> parameters{4, 3, 4, 2, 1};

  Circuit<Layout, CorrelationLevel::L2> circuit(parameters);
  circuit.counters.measurement = 2;

  const std::vector<double> args;
  const std::vector<stim::GateTarget> targets{stim::GateTarget::rec(-2),
                                              stim::GateTarget::rec(-1)};

  const stim::CircuitInstruction op(
      stim::GateType::DETECTOR, args, targets, std::string_view{});

  circuit.applyOp(op);

  REQUIRE(circuit.counters.detector == 1);
  REQUIRE(circuit.sensitivityMap.vals(0, 0) == 1ULL);
  REQUIRE(circuit.sensitivityMap.vals(1, 0) == 1ULL);
}

template <typename Layout>
static void testCircuitApplyOpObservableInclude() {
  constexpr CircuitParameters<CorrelationLevel::L2> parameters{4, 3, 4, 2, 1};

  Circuit<Layout, CorrelationLevel::L2> circuit(parameters);
  circuit.counters.measurement = 2;
  circuit.counters.detector = 1;

  const std::vector<double> args{0};
  const std::vector<stim::GateTarget> targets{stim::GateTarget::rec(-1)};

  const stim::CircuitInstruction op(
      stim::GateType::OBSERVABLE_INCLUDE, args, targets, std::string_view{});

  circuit.applyOp(op);

  REQUIRE(circuit.counters.detector == 2);
  REQUIRE(circuit.sensitivityMap.vals(1, 0) == 2ULL);
}

template <typename Layout>
static void testCircuitApplyOpTick() {
  constexpr CircuitParameters<CorrelationLevel::L2> parameters{4, 3, 4, 2, 1};

  Circuit<Layout, CorrelationLevel::L2> circuit(parameters);

  const std::vector<double> args;
  const std::vector<stim::GateTarget> targets;

  const stim::CircuitInstruction op(
      stim::GateType::TICK, args, targets, std::string_view{});

  circuit.applyOp(op);

  REQUIRE(circuit.counters.layer == 1);
}

template <typename Layout>
static void testCircuitApplyOpM() {
  constexpr CircuitParameters<CorrelationLevel::L2> parameters{4, 3, 4, 2, 1};

  Circuit<Layout, CorrelationLevel::L2> circuit(parameters);
  circuit.counters.layer = 1;

  const std::vector<double> args{0.001};
  const std::vector<stim::GateTarget> targets{stim::GateTarget::qubit(0)};

  const stim::CircuitInstruction op(
      stim::GateType::M, args, targets, std::string_view{});

  circuit.applyOp(op);

  REQUIRE_NEAR(circuit.stepg.probs[32], 0.001);
  REQUIRE(circuit.sensitivityMap.keys[0] == 32);
  REQUIRE(circuit.counters.measurement == 1);
}

template <typename Layout>
static void testCircuitApplyOpR() {
  constexpr CircuitParameters<CorrelationLevel::L2> parameters{4, 3, 4, 2, 1};

  Circuit<Layout, CorrelationLevel::L2> circuit(parameters);

  constexpr uint32_t q = 0;
  constexpr uint32_t l = 0;

  const uint32_t x = q * Mixer<CorrelationLevel::L2>::numNodesPerQubit;
  const uint32_t z = q * Mixer<CorrelationLevel::L2>::numNodesPerQubit + 1;

  const STCoord xLo(x, l);
  const STCoord xUp(x, l + 1);
  const STCoord zLo(z, l);
  const STCoord zUp(z, l + 1);

  circuit.stepg.addFlow(xLo, xUp);
  circuit.stepg.addFlow(zLo, zUp);

  const std::vector<double> args;
  const std::vector<stim::GateTarget> targets{stim::GateTarget::qubit(q)};

  const stim::CircuitInstruction op(
      stim::GateType::R, args, targets, std::string_view{});

  circuit.applyOp(op);

  REQUIRE(getLower(circuit.stepg.graph[xLo.getIndex(32)]) == UINT32_MAX);
  REQUIRE(getLower(circuit.stepg.graph[zLo.getIndex(32)]) == UINT32_MAX);
}

template <typename Layout>
static void testCircuitApplyOpCX() {
  constexpr CircuitParameters<CorrelationLevel::L2> parameters{4, 3, 4, 2, 1};

  Circuit<Layout, CorrelationLevel::L2> circuit(parameters);

  constexpr uint32_t c = 0;
  constexpr uint32_t t = 1;
  constexpr uint32_t l = 0;

  const uint32_t cX = c * Mixer<CorrelationLevel::L2>::numNodesPerQubit;
  const uint32_t cZ = c * Mixer<CorrelationLevel::L2>::numNodesPerQubit + 1;
  const uint32_t tX = t * Mixer<CorrelationLevel::L2>::numNodesPerQubit;
  const uint32_t tZ = t * Mixer<CorrelationLevel::L2>::numNodesPerQubit + 1;

  const STCoord cXLo(cX, l);
  const STCoord tXUp(tX, l + 1);
  const STCoord tZLo(tZ, l);
  const STCoord cZUp(cZ, l + 1);

  const std::vector<double> args;
  const std::vector<stim::GateTarget> targets{stim::GateTarget::qubit(c),
                                              stim::GateTarget::qubit(t)};

  const stim::CircuitInstruction op(
      stim::GateType::CX, args, targets, std::string_view{});

  circuit.applyOp(op);

  REQUIRE(getLower(circuit.stepg.graph[cXLo.getIndex(32)]) ==
          tXUp.getIndex(32));
  REQUIRE(getLower(circuit.stepg.graph[tZLo.getIndex(32)]) ==
          cZUp.getIndex(32));
}

template <typename Layout>
static void testCircuitApplyOpH() {
  constexpr CircuitParameters<CorrelationLevel::L2> parameters{4, 3, 4, 2, 1};

  Circuit<Layout, CorrelationLevel::L2> circuit(parameters);

  constexpr uint32_t q = 0;
  constexpr uint32_t l = 0;

  const uint32_t x = q * Mixer<CorrelationLevel::L2>::numNodesPerQubit;
  const uint32_t z = q * Mixer<CorrelationLevel::L2>::numNodesPerQubit + 1;

  const STCoord xLo(x, l);
  const STCoord xUp(x, l + 1);
  const STCoord zLo(z, l);
  const STCoord zUp(z, l + 1);

  circuit.stepg.addFlow(xLo, xUp);
  circuit.stepg.addFlow(zLo, zUp);

  const std::vector<double> args;
  const std::vector<stim::GateTarget> targets{stim::GateTarget::qubit(q)};

  const stim::CircuitInstruction op(
      stim::GateType::H, args, targets, std::string_view{});

  circuit.applyOp(op);

  REQUIRE(getLower(circuit.stepg.graph[xLo.getIndex(32)]) == zUp.getIndex(32));
  REQUIRE(getLower(circuit.stepg.graph[zLo.getIndex(32)]) == xUp.getIndex(32));
}

template <typename Layout>
static void testCircuitApplyOpDepolarize1() {
  constexpr CircuitParameters<CorrelationLevel::L2> parameters{4, 3, 4, 2, 1};

  Circuit<Layout, CorrelationLevel::L2> circuit(parameters);
  circuit.counters.layer = 1;

  constexpr uint32_t q = 0;
  constexpr uint32_t l = 1;
  constexpr double p = 0.001;

  const uint32_t base = q * Mixer<CorrelationLevel::L2>::numNodesPerQubit;
  const uint32_t x = base + getNodeOffset(NodeType::X);
  const uint32_t y = base + getNodeOffset(NodeType::Y);
  const uint32_t z = base + getNodeOffset(NodeType::Z);

  const STCoord xCoord(x, l);
  const STCoord yCoord(y, l - 1);
  const STCoord zCoord(z, l);

  const uint32_t xIndex = xCoord.getIndex(32);
  const uint32_t yIndex = yCoord.getIndex(32);
  const uint32_t zIndex = zCoord.getIndex(32);

  const std::vector<double> args{p};
  const std::vector<stim::GateTarget> targets{stim::GateTarget::qubit(q)};

  const stim::CircuitInstruction op(
      stim::GateType::DEPOLARIZE1, args, targets, std::string_view{});

  circuit.applyOp(op);

  // --- Check Y error ---

  REQUIRE(getLower(circuit.stepg.graph[yIndex]) == xIndex);
  REQUIRE(getUpper(circuit.stepg.graph[yIndex]) == zIndex);

  REQUIRE_NEAR(circuit.stepg.probs[yIndex], 0.00033344451858030588);
}

template <typename Layout>
static void testCircuitApplyOpDepolarize2() {
  constexpr CircuitParameters<CorrelationLevel::L2> parameters{4, 3, 4, 2, 1};

  Circuit<Layout, CorrelationLevel::L2> circuit(parameters);
  circuit.counters.layer = 2;

  constexpr uint32_t q0 = 0;
  constexpr uint32_t q1 = 1;
  constexpr uint32_t l = 2;
  constexpr double p = 0.001;

  const uint32_t base0 = q0 * Mixer<CorrelationLevel::L2>::numNodesPerQubit;
  const uint32_t base1 = q1 * Mixer<CorrelationLevel::L2>::numNodesPerQubit;
  const uint32_t y0 = base0 + getNodeOffset(NodeType::Y);
  const uint32_t y1 = base1 + getNodeOffset(NodeType::Y);
  const uint32_t yy = base0 + getNodeOffset(NodeType::YY);

  const STCoord y0Coord(y0, l - 1);
  const STCoord y1Coord(y1, l - 1);
  const STCoord yyCoord(yy, l - 2);

  const uint32_t yyIndex = yyCoord.getIndex(32);
  const uint32_t y0Index = y0Coord.getIndex(32);
  const uint32_t y1Index = y1Coord.getIndex(32);

  const std::vector<double> args{p};
  const std::vector<stim::GateTarget> targets{stim::GateTarget::qubit(q0),
                                              stim::GateTarget::qubit(q1)};

  const stim::CircuitInstruction op(
      stim::GateType::DEPOLARIZE2, args, targets, std::string_view{});

  circuit.applyOp(op);

  // --- Check YY error ---

  REQUIRE(getLower(circuit.stepg.graph[yyIndex]) == y0Index);
  REQUIRE(getUpper(circuit.stepg.graph[yyIndex]) == y1Index);

  REQUIRE_NEAR(circuit.stepg.probs[yyIndex], 6.6697798534409714e-05);
}

template <typename Layout>
static void testCircuitApplyOpXError() {
  constexpr CircuitParameters<CorrelationLevel::L2> parameters{4, 3, 4, 2, 1};

  Circuit<Layout, CorrelationLevel::L2> circuit(parameters);
  circuit.counters.layer = 1;

  constexpr uint32_t q = 0;
  constexpr uint32_t l = 1;
  constexpr double p = 0.001;

  const uint32_t base = q * Mixer<CorrelationLevel::L2>::numNodesPerQubit;

  const uint32_t x = base + getNodeOffset(NodeType::X);
  const STCoord xCoord(x, l);

  const uint32_t xIndex = xCoord.getIndex(32);

  const std::vector<double> args{p};
  const std::vector<stim::GateTarget> targets{stim::GateTarget::qubit(q)};

  const stim::CircuitInstruction op(
      stim::GateType::X_ERROR, args, targets, std::string_view{});

  circuit.applyOp(op);

  REQUIRE_NEAR(circuit.stepg.probs[xIndex], p);
}

template <typename Layout>
static void testCircuitApplyOpUnsupported() {
  constexpr CircuitParameters<CorrelationLevel::L2> parameters{4, 3, 4, 2, 1};

  Circuit<Layout, CorrelationLevel::L2> circuit(parameters);

  const std::vector<double> args;
  const std::vector<stim::GateTarget> targets{stim::GateTarget::qubit(0)};

  const stim::CircuitInstruction op(
      stim::GateType::I, args, targets, std::string_view{});

  bool threw = false;
  try {
    circuit.applyOp(op);
  } catch (const std::invalid_argument &) {
    threw = true;
  }

  REQUIRE(threw);
}

template <typename Layout>
static void runCircuitTests() {
  // Constructor
  testCircuitConstructor<Layout>();

  // Circuit::applyOp [DETECTOR]
  testCircuitApplyOpDetector<Layout>();

  // Circuit::applyOp [OBSERVABLE_INCLUDE]
  testCircuitApplyOpObservableInclude<Layout>();

  // Circuit::applyOp [TICK]
  testCircuitApplyOpTick<Layout>();

  // Circuit::applyOp [M]
  testCircuitApplyOpM<Layout>();

  // Circuit::applyOp [R]
  testCircuitApplyOpR<Layout>();

  // Circuit::applyOp [CX]
  testCircuitApplyOpCX<Layout>();

  // Circuit::applyOp [H]
  testCircuitApplyOpH<Layout>();

  // Circuit::applyOp [DEPOLARIZE1]
  testCircuitApplyOpDepolarize1<Layout>();

  // Circuit::applyOp [DEPOLARIZE2]
  testCircuitApplyOpDepolarize2<Layout>();

  // Circuit::applyOp [X_ERROR]
  testCircuitApplyOpXError<Layout>();

  // Circuit::applyOp [unsupported]
  testCircuitApplyOpUnsupported<Layout>();
}

int main() {
  // --- Circuit ---

  runCircuitTests<RowMajorLayout>();
  runCircuitTests<ColMajorLayout>();

  return 0;
}

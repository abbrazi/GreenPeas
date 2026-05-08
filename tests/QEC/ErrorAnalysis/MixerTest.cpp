/// Standard headers
#include <cstdint>

/// Helper headers
#include "../../Helpers/Macros.hpp"

/// Project headers
#include "GreenPeas/Core/Words.hpp"
#include "GreenPeas/QEC/ErrorAnalysis/Mixer.hpp"

using namespace gp;

template <CorrelationLevel L>
static void testMixerApplyRRemovesPersistenceFlows() {
  STEPG stepg(2, 32);

  constexpr uint32_t q = 0;
  constexpr uint32_t l = 0;

  const uint32_t x = q * Mixer<L>::numNodesPerQubit;
  const uint32_t z = q * Mixer<L>::numNodesPerQubit + 1;

  const STCoord xLo(x, l);
  const STCoord xUp(x, l + 1);
  const STCoord zLo(z, l);
  const STCoord zUp(z, l + 1);

  stepg.addFlow(xLo, xUp);
  stepg.addFlow(zLo, zUp);

  Mixer<L>::applyR(stepg, q, l);

  REQUIRE(getLower(stepg.graph[xLo.getIndex(32)]) == UINT32_MAX);
  REQUIRE(getLower(stepg.graph[zLo.getIndex(32)]) == UINT32_MAX);
}

template <CorrelationLevel L>
static void testMixerApplyCXAddsPropagationFlows() {
  STEPG stepg(2, 32);

  constexpr uint32_t c = 0;
  constexpr uint32_t t = 1;
  constexpr uint32_t l = 0;

  const uint32_t cX = c * Mixer<L>::numNodesPerQubit;
  const uint32_t cZ = c * Mixer<L>::numNodesPerQubit + 1;
  const uint32_t tX = t * Mixer<L>::numNodesPerQubit;
  const uint32_t tZ = t * Mixer<L>::numNodesPerQubit + 1;

  const STCoord cXLo(cX, l);
  const STCoord tXUp(tX, l + 1);
  const STCoord tZLo(tZ, l);
  const STCoord cZUp(cZ, l + 1);

  Mixer<L>::applyCX(stepg, c, t, l);

  REQUIRE(getLower(stepg.graph[cXLo.getIndex(32)]) == tXUp.getIndex(32));
  REQUIRE(getLower(stepg.graph[tZLo.getIndex(32)]) == cZUp.getIndex(32));
}

template <CorrelationLevel L>
static void testMixerApplyHReversesPersistenceFlows() {
  STEPG stepg(2, 32);

  constexpr uint32_t q = 0;
  constexpr uint32_t l = 0;

  const uint32_t x = q * Mixer<L>::numNodesPerQubit;
  const uint32_t z = q * Mixer<L>::numNodesPerQubit + 1;

  const STCoord xLo(x, l);
  const STCoord xUp(x, l + 1);
  const STCoord zLo(z, l);
  const STCoord zUp(z, l + 1);

  stepg.addFlow(xLo, xUp);
  stepg.addFlow(zLo, zUp);

  Mixer<L>::applyH(stepg, q, l);

  REQUIRE(getLower(stepg.graph[xLo.getIndex(32)]) == zUp.getIndex(32));
  REQUIRE(getLower(stepg.graph[zLo.getIndex(32)]) == xUp.getIndex(32));
}

static void testMixerApplyDepolarize1() {
  STEPG stepg(3, 32);

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

  const uint32_t xIndex = xCoord.getIndex(stepg.numNodesPerLayer);
  const uint32_t yIndex = yCoord.getIndex(stepg.numNodesPerLayer);
  const uint32_t zIndex = zCoord.getIndex(stepg.numNodesPerLayer);

  Mixer<CorrelationLevel::L2>::applyDepolarize1(stepg, q, l, p);

  // --- Check Y error ---

  REQUIRE(getLower(stepg.graph[yIndex]) == xIndex);
  REQUIRE(getUpper(stepg.graph[yIndex]) == zIndex);

  REQUIRE_NEAR(stepg.probs[yIndex], 0.00033344451858030588);
}

static void testMixerApplyDepolarize2() {
  STEPG stepg(3, 32);

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

  const uint32_t yyIndex = yyCoord.getIndex(stepg.numNodesPerLayer);
  const uint32_t y0Index = y0Coord.getIndex(stepg.numNodesPerLayer);
  const uint32_t y1Index = y1Coord.getIndex(stepg.numNodesPerLayer);

  Mixer<CorrelationLevel::L2>::applyDepolarize2(stepg, q0, q1, l, p);

  // --- Check YY error ---

  REQUIRE(getLower(stepg.graph[yyIndex]) == y0Index);
  REQUIRE(getUpper(stepg.graph[yyIndex]) == y1Index);

  REQUIRE_NEAR(stepg.probs[yyIndex], 6.6697798534409714e-05);
}

auto main() -> int {
  // --- CorrelationLevel::L0 ---

  // Mixer::applyR
  testMixerApplyRRemovesPersistenceFlows<CorrelationLevel::L0>();

  // Mixer::applyCX
  testMixerApplyCXAddsPropagationFlows<CorrelationLevel::L0>();

  // Mixer::applyH
  testMixerApplyHReversesPersistenceFlows<CorrelationLevel::L0>();

  // --- CorrelationLevel::L1 ---

  // Mixer::applyR
  testMixerApplyRRemovesPersistenceFlows<CorrelationLevel::L1>();

  // Mixer::applyCX
  testMixerApplyCXAddsPropagationFlows<CorrelationLevel::L1>();

  // Mixer::applyH
  testMixerApplyHReversesPersistenceFlows<CorrelationLevel::L1>();

  // --- CorrelationLevel::L2 ---

  // Mixer::applyR
  testMixerApplyRRemovesPersistenceFlows<CorrelationLevel::L2>();

  // Mixer::applyCX
  testMixerApplyCXAddsPropagationFlows<CorrelationLevel::L2>();

  // Mixer::applyH
  testMixerApplyHReversesPersistenceFlows<CorrelationLevel::L2>();

  // Mixer::applyDepolarize1
  testMixerApplyDepolarize1();

  // Mixer::applyDepolarize2
  testMixerApplyDepolarize2();

  return 0;
}

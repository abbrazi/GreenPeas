#ifndef GREENPEAS_QEC_ERRORANALYSIS_MIXER_HPP
#define GREENPEAS_QEC_ERRORANALYSIS_MIXER_HPP

/// Project headers
#include <cmath>

#include "GreenPeas/Common.hpp"
#include "GreenPeas/QEC/ErrorAnalysis/STEPG.hpp"

namespace gp {

/// @brief Controls the Pauli errors applied by the error mixer.
enum class CorrelationLevel : uint32_t {
  /// X, Z, IX, IZ, XI, XX, ZI, ZZ
  L0 = 0,
  /// X, Y, Z, IX, IY, IZ, XI, XX, XZ, YI, ZI, ZX, ZZ
  L1 = 1,
  /// X, Y, Z, IX, IY, IZ, XI, XX, XY, XZ, YI, YX, YY, YZ, ZI, ZX, ZY, ZZ
  L2 = 2,
};

/// @brief Get the number of Pauli error nodes required per qubit for the given:
/// @param level The correlation level.
HOST inline constexpr uint32_t getNumNodesPerQubit(CorrelationLevel level) {
  switch (level) {
  case CorrelationLevel::L0:
    return 3;
  case CorrelationLevel::L1:
    return 5;
  case CorrelationLevel::L2:
    return 8;
  }
  throw std::runtime_error("Invalid correlation level.");
}

/// @brief Type of a single- or two-qubit Pauli error node.
enum class NodeType : uint32_t {
  X = 0,
  Y,
  Z,
  IX,
  IY,
  IZ,
  XI,
  XX,
  XY,
  XZ,
  YI,
  YX,
  YY,
  YZ,
  ZI,
  ZX,
  ZY,
  ZZ
};

/// @brief Spatial offset of each Pauli error node type.
inline constexpr uint32_t nodeOffset[] = {
    0, // X
    3, // Y
    1, // Z
    0, // IX
    3, // IY
    1, // IZ
    0, // XI
    2, // XX
    5, // XY
    4, // XZ
    3, // YI
    5, // YX
    6, // YY
    7, // YZ
    1, // ZI
    4, // ZX
    7, // ZY
    2, // ZZ
};

/// @brief Get the spatial offset for the given:
/// @param type Pauli error node type.
HOST inline constexpr uint32_t getNodeOffset(NodeType type) {
  return nodeOffset[static_cast<uint32_t>(type)];
}

/// @brief Error mixer for gates and noise channels.
/// @tparam Level Correlation level.
template <CorrelationLevel Level>
struct Mixer {
  /// @brief Number of Pauli error nodes per qubit.
  static constexpr uint32_t numNodesPerQubit = getNumNodesPerQubit(Level);

  /// @brief Initialise STEPG for a circuit with @p numQubits and @p numLayers.
  /// @param stepg STEPG to Initialise.
  /// @param numQubits Number of qubits in the circuit.
  /// @param numLayers Number of layers in the circuit,
  HOST static void
  initialise(STEPG &stepg, uint32_t numQubits, uint32_t numLayers) {
    for (uint32_t layer = 0; layer + 1 < numLayers; ++layer) {
      for (uint32_t qubit = 0; qubit < numQubits; ++qubit) {
        const uint32_t base = qubit * numNodesPerQubit;
        const uint32_t x = base + getNodeOffset(NodeType::X);
        const uint32_t z = base + getNodeOffset(NodeType::Z);
        stepg.addFlow({x, layer}, {x, layer + 1});
        stepg.addFlow({z, layer}, {z, layer + 1});
      }
    }
  }

  /// @brief Apply a reset gate on qubit @p q at layer @p l.
  /// @param stepg STEPG to update.
  /// @param q Qubit index.
  /// @param l Time layer index.
  HOST static void applyR(STEPG &stepg, uint32_t q, uint32_t l) {
    const uint32_t base = q * numNodesPerQubit;
    const uint32_t x = base;
    const uint32_t z = base + 1;
    stepg.removeFlow({x, l}, {x, l + 1});
    stepg.removeFlow({z, l}, {z, l + 1});
  }

  /// @brief Apply a controlled-X gate between @p c and @p t at layer @p l.
  /// @param stepg STEPG to update.
  /// @param c Control qubit index.
  /// @param t Target qubit index.
  /// @param l Time layer index.
  HOST static void applyCX(STEPG &stepg, uint32_t c, uint32_t t, uint32_t l) {
    const uint32_t cBase = c * numNodesPerQubit;
    const uint32_t tBase = t * numNodesPerQubit;
    const uint32_t cX = cBase;
    const uint32_t cZ = cBase + 1;
    const uint32_t tX = tBase;
    const uint32_t tZ = tBase + 1;
    stepg.addFlow({cX, l}, {tX, l + 1});
    stepg.addFlow({tZ, l}, {cZ, l + 1});
  }

  /// @brief Apply a Hadamard gate on qubit @p q at layer @p l.
  /// @param stepg STEPG to update.
  /// @param q Qubit index.
  /// @param l Time layer index.
  HOST static void applyH(STEPG &stepg, uint32_t q, uint32_t l) {
    const uint32_t base = q * numNodesPerQubit;
    const uint32_t x = base;
    const uint32_t z = base + 1;
    stepg.removeFlow({x, l}, {x, l + 1});
    stepg.removeFlow({z, l}, {z, l + 1});
    stepg.addFlow({x, l}, {z, l + 1});
    stepg.addFlow({z, l}, {x, l + 1});
  }

  /// @brief Apply a single-qubit depolarizing channel on qubit @p q.
  /// @param stepg STEPG to update.
  /// @param q Qubit index.
  /// @param l Time layer index.
  /// @param p Total depolarizing probability (must be <= 0.75).
  /// @throws std::runtime_error If @p p > 0.75.
  HOST static void
  applyDepolarize1(STEPG &stepg, uint32_t q, uint32_t l, double p) {
    if (p > 0.75) {
      throw std::runtime_error("Mixer: invalid DEP1 p > 0.75.");
    }

    const double splitP = 0.5 - 0.5 * std::sqrt(1 - (4 * p) / 3);

    applyXError(stepg, q, l, splitP);
    applyZError(stepg, q, l, splitP);
    applyYError(stepg, q, l, splitP);
  }

  /// @brief Apply a two-qubit depolarizing channel on qubits (@p q0, @p q1).
  /// @param stepg STEPG to update.
  /// @param q0 First qubit index.
  /// @param q1 Second qubit index.
  /// @param l Time layer index.
  /// @param p Total depolarizing probability (must be <= 0.9375).
  /// @throws std::runtime_error If @p p > 0.9375.
  HOST static void applyDepolarize2(
      STEPG &stepg, uint32_t q0, uint32_t q1, uint32_t l, double p) {
    if (p > 0.9375) {
      throw std::runtime_error("Mixer: invalid DEP2 p > 0.9375.");
    }

    const double splitP = 0.5 - 0.5 * std::pow(1 - (16 * p) / 15, 0.125);

    applyIXError(stepg, q0, q1, l, splitP);
    applyIYError(stepg, q0, q1, l, splitP);
    applyIZError(stepg, q0, q1, l, splitP);
    applyXIError(stepg, q0, q1, l, splitP);
    applyXXError(stepg, q0, q1, l, splitP);
    applyXYError(stepg, q0, q1, l, splitP);
    applyXZError(stepg, q0, q1, l, splitP);
    applyYIError(stepg, q0, q1, l, splitP);
    applyYXError(stepg, q0, q1, l, splitP);
    applyYYError(stepg, q0, q1, l, splitP);
    applyYZError(stepg, q0, q1, l, splitP);
    applyZIError(stepg, q0, q1, l, splitP);
    applyZXError(stepg, q0, q1, l, splitP);
    applyZYError(stepg, q0, q1, l, splitP);
    applyZZError(stepg, q0, q1, l, splitP);
  }

  /// @brief Apply an X error on qubit @p q at layer @p l.
  /// @param stepg STEPG to update.
  /// @param q Qubit index.
  /// @param l Time layer index.
  /// @param p Error probability.
  /// @return Linear index of the updated error node.
  HOST static auto applyXError(STEPG &stepg, uint32_t q, uint32_t l, double p)
      -> uint32_t {
    const STCoord coord{q * numNodesPerQubit + getNodeOffset(NodeType::X), l};
    stepg.mergeProbabilities(coord, p);
    return coord.getIndex(stepg.numNodesPerLayer);
  }

  /// @brief Apply a Y error on qubit @p q at layer @p l (L1+ only).
  /// @param stepg STEPG to update.
  /// @param q Qubit index.
  /// @param l Time layer index.
  /// @param p Error probability.
  /// @throws std::runtime_error If @p l < 1 at L1+.
  HOST static void applyYError(STEPG &stepg, uint32_t q, uint32_t l, double p) {
    if constexpr (Level >= CorrelationLevel::L1) {
      if (l < 1) {
        throw std::runtime_error("Mixer: cannot apply Y error in layer l < 1");
      }
      const uint32_t base = q * numNodesPerQubit;
      const uint32_t x = base + getNodeOffset(NodeType::X);
      const uint32_t y = base + getNodeOffset(NodeType::Y);
      const uint32_t z = base + getNodeOffset(NodeType::Z);
      stepg.addFlow({y, l - 1}, {x, l});
      stepg.addFlow({y, l - 1}, {z, l});
      stepg.mergeProbabilities({y, l - 1}, p);
    }
  }

  /// @brief Apply a Z error on qubit @p q at layer @p l.
  /// @param stepg STEPG to update.
  /// @param q Qubit index.
  /// @param l Time layer index.
  /// @param p Error probability.
  /// @return Linear index of the updated error node.
  HOST static auto applyZError(STEPG &stepg, uint32_t q, uint32_t l, double p)
      -> uint32_t {
    const STCoord coord{q * numNodesPerQubit + getNodeOffset(NodeType::Z), l};
    stepg.mergeProbabilities(coord, p);
    return coord.getIndex(stepg.numNodesPerLayer);
  }

  /// @brief Apply an IX error on qubits (@p q0, @p q1) at layer @p l.
  /// @param stepg STEPG to update.
  /// @param q0 First qubit index.
  /// @param q1 Second qubit index.
  /// @param l Time layer index.
  /// @param p Error probability.
  HOST static void
  applyIXError(STEPG &stepg, uint32_t q0, uint32_t q1, uint32_t l, double p) {
    (void)q0;
    applyXError(stepg, q1, l, p);
  }

  /// @brief Apply an IY error on qubits (@p q0, @p q1) at layer @p l.
  /// @param stepg STEPG to update.
  /// @param q0 First qubit index.
  /// @param q1 Second qubit index.
  /// @param l Time layer index.
  /// @param p Error probability.
  HOST static void
  applyIYError(STEPG &stepg, uint32_t q0, uint32_t q1, uint32_t l, double p) {
    (void)q0;
    applyYError(stepg, q1, l, p);
  }

  /// @brief Apply an IZ error on qubits (@p q0, @p q1) at layer @p l.
  /// @param stepg STEPG to update.
  /// @param q0 First qubit index.
  /// @param q1 Second qubit index.
  /// @param l Time layer index.
  /// @param p Error probability.
  HOST static void
  applyIZError(STEPG &stepg, uint32_t q0, uint32_t q1, uint32_t l, double p) {
    (void)q0;
    applyZError(stepg, q1, l, p);
  }

  /// @brief Apply an XI error on qubits (@p q0, @p q1) at layer @p l.
  /// @param stepg STEPG to update.
  /// @param q0 First qubit index.
  /// @param q1 Second qubit index.
  /// @param l Time layer index.
  /// @param p Error probability.
  HOST static void
  applyXIError(STEPG &stepg, uint32_t q0, uint32_t q1, uint32_t l, double p) {
    (void)q1;
    applyXError(stepg, q0, l, p);
  }

  /// @brief Apply an XX error on qubits (@p q0, @p q1) at layer @p l.
  /// @param stepg STEPG to update.
  /// @param q0 First qubit index.
  /// @param q1 Second qubit index.
  /// @param l Time layer index.
  /// @param p Error probability.
  /// @throws std::runtime_error If @p l < 1.
  HOST static void
  applyXXError(STEPG &stepg, uint32_t q0, uint32_t q1, uint32_t l, double p) {
    if (l < 1) {
      throw std::runtime_error("Mixer: cannot apply XX error in layer l < 1");
    }
    const uint32_t base0 = q0 * numNodesPerQubit;
    const uint32_t base1 = q1 * numNodesPerQubit;
    const uint32_t x0 = base0 + getNodeOffset(NodeType::X);
    const uint32_t x1 = base1 + getNodeOffset(NodeType::X);
    const uint32_t xx = base0 + getNodeOffset(NodeType::XX);
    stepg.addFlow({xx, l - 1}, {x0, l});
    stepg.addFlow({xx, l - 1}, {x1, l});
    stepg.mergeProbabilities({xx, l - 1}, p);
  }

  /// @brief Apply an XY error on qubits (@p q0, @p q1) at layer @p l (L2 only).
  /// @param stepg STEPG to update.
  /// @param q0 First qubit index.
  /// @param q1 Second qubit index.
  /// @param l Time layer index.
  /// @param p Error probability.
  /// @throws std::runtime_error If @p l < 2 at L2.
  HOST static void
  applyXYError(STEPG &stepg, uint32_t q0, uint32_t q1, uint32_t l, double p) {
    if constexpr (Level == CorrelationLevel::L2) {
      if (l < 2) {
        throw std::runtime_error("Mixer: cannot apply XY error in layer l < 2");
      }
      const uint32_t base0 = q0 * numNodesPerQubit;
      const uint32_t base1 = q1 * numNodesPerQubit;
      const uint32_t x0 = base0 + getNodeOffset(NodeType::X);
      const uint32_t y1 = base1 + getNodeOffset(NodeType::Y);
      const uint32_t xy = base0 + getNodeOffset(NodeType::XY);
      stepg.addFlow({xy, l - 2}, {x0, l});
      stepg.addFlow({xy, l - 2}, {y1, l - 1});
      stepg.mergeProbabilities({xy, l - 2}, p);
    }
  }

  /// @brief Apply an XZ error on qubits (@p q0, @p q1) at layer @p l (L1+
  /// only).
  /// @param stepg STEPG to update.
  /// @param q0 First qubit index.
  /// @param q1 Second qubit index.
  /// @param l Time layer index.
  /// @param p Error probability.
  /// @throws std::runtime_error If @p l < 1 at L1+.
  HOST static void
  applyXZError(STEPG &stepg, uint32_t q0, uint32_t q1, uint32_t l, double p) {
    if constexpr (Level >= CorrelationLevel::L1) {
      if (l < 1) {
        throw std::runtime_error("Mixer: cannot apply XZ error in layer l < 1");
      }
      const uint32_t base0 = q0 * numNodesPerQubit;
      const uint32_t base1 = q1 * numNodesPerQubit;
      const uint32_t x0 = base0 + getNodeOffset(NodeType::X);
      const uint32_t z1 = base1 + getNodeOffset(NodeType::Z);
      const uint32_t xz = base0 + getNodeOffset(NodeType::XZ);
      stepg.addFlow({xz, l - 1}, {x0, l});
      stepg.addFlow({xz, l - 1}, {z1, l});
      stepg.mergeProbabilities({xz, l - 1}, p);
    }
  }

  /// @brief Apply a YI error on qubits (@p q0, @p q1) at layer @p l.
  /// @param stepg STEPG to update.
  /// @param q0 First qubit index.
  /// @param q1 Second qubit index.
  /// @param l Time layer index.
  /// @param p Error probability.
  HOST static void
  applyYIError(STEPG &stepg, uint32_t q0, uint32_t q1, uint32_t l, double p) {
    (void)q1;
    applyYError(stepg, q0, l, p);
  }

  /// @brief Apply a YX error on qubits (@p q0, @p q1) at layer @p l (L2 only).
  /// @param stepg STEPG to update.
  /// @param q0 First qubit index.
  /// @param q1 Second qubit index.
  /// @param l Time layer index.
  /// @param p Error probability.
  /// @throws std::runtime_error If @p l < 2 at L2.
  HOST static void
  applyYXError(STEPG &stepg, uint32_t q0, uint32_t q1, uint32_t l, double p) {
    if constexpr (Level == CorrelationLevel::L2) {
      if (l < 2) {
        throw std::runtime_error("Mixer: cannot apply YX error in layer l < 2");
      }
      const uint32_t base0 = q0 * numNodesPerQubit;
      const uint32_t base1 = q1 * numNodesPerQubit;
      const uint32_t y0 = base0 + getNodeOffset(NodeType::Y);
      const uint32_t x1 = base1 + getNodeOffset(NodeType::X);
      const uint32_t yx = base1 + getNodeOffset(NodeType::YX);
      stepg.addFlow({yx, l - 2}, {y0, l - 1});
      stepg.addFlow({yx, l - 2}, {x1, l});
      stepg.mergeProbabilities({yx, l - 2}, p);
    }
  }

  /// @brief Apply a YY error on qubits (@p q0, @p q1) at layer @p l (L2 only).
  /// @param stepg STEPG to update.
  /// @param q0 First qubit index.
  /// @param q1 Second qubit index.
  /// @param l Time layer index.
  /// @param p Error probability.
  /// @throws std::runtime_error If @p l < 2 at L2.
  HOST static void
  applyYYError(STEPG &stepg, uint32_t q0, uint32_t q1, uint32_t l, double p) {
    if constexpr (Level == CorrelationLevel::L2) {
      if (l < 2) {
        throw std::runtime_error("Mixer: cannot apply YY error in layer l < 2");
      }
      const uint32_t base0 = q0 * numNodesPerQubit;
      const uint32_t base1 = q1 * numNodesPerQubit;
      const uint32_t y0 = base0 + getNodeOffset(NodeType::Y);
      const uint32_t y1 = base1 + getNodeOffset(NodeType::Y);
      const uint32_t yy = base0 + getNodeOffset(NodeType::YY);
      stepg.addFlow({yy, l - 2}, {y0, l - 1});
      stepg.addFlow({yy, l - 2}, {y1, l - 1});
      stepg.mergeProbabilities({yy, l - 2}, p);
    }
  }

  /// @brief Apply a YZ error on qubits (@p q0, @p q1) at layer @p l (L2 only).
  /// @param stepg STEPG to update.
  /// @param q0 First qubit index.
  /// @param q1 Second qubit index.
  /// @param l Time layer index.
  /// @param p Error probability.
  /// @throws std::runtime_error If @p l < 2 at L2.
  HOST static void
  applyYZError(STEPG &stepg, uint32_t q0, uint32_t q1, uint32_t l, double p) {
    if constexpr (Level == CorrelationLevel::L2) {
      if (l < 2) {
        throw std::runtime_error("Mixer: cannot apply YZ error in layer l < 2");
      }
      const uint32_t base0 = q0 * numNodesPerQubit;
      const uint32_t base1 = q1 * numNodesPerQubit;
      const uint32_t y0 = base0 + getNodeOffset(NodeType::Y);
      const uint32_t z1 = base1 + getNodeOffset(NodeType::Z);
      const uint32_t yz = base1 + getNodeOffset(NodeType::YZ);
      stepg.addFlow({yz, l - 2}, {y0, l - 1});
      stepg.addFlow({yz, l - 2}, {z1, l});
      stepg.mergeProbabilities({yz, l - 2}, p);
    }
  }

  /// @brief Apply a ZI error on qubits (@p q0, @p q1) at layer @p l.
  /// @param stepg STEPG to update.
  /// @param q0 First qubit index.
  /// @param q1 Second qubit index.
  /// @param l Time layer index.
  /// @param p Error probability.
  HOST static void
  applyZIError(STEPG &stepg, uint32_t q0, uint32_t q1, uint32_t l, double p) {
    (void)q1;
    applyZError(stepg, q0, l, p);
  }

  /// @brief Apply a ZX error on qubits (@p q0, @p q1) at layer @p l (L1+ only).
  /// @param stepg STEPG to update.
  /// @param q0 First qubit index.
  /// @param q1 Second qubit index.
  /// @param l Time layer index.
  /// @param p Error probability.
  /// @throws std::runtime_error If @p l < 1 at L1+.
  HOST static void
  applyZXError(STEPG &stepg, uint32_t q0, uint32_t q1, uint32_t l, double p) {
    if constexpr (Level >= CorrelationLevel::L1) {
      if (l < 1) {
        throw std::runtime_error("Mixer: cannot apply ZX error in layer l < 1");
      }
      const uint32_t base0 = q0 * numNodesPerQubit;
      const uint32_t base1 = q1 * numNodesPerQubit;
      const uint32_t z0 = base0 + getNodeOffset(NodeType::Z);
      const uint32_t x1 = base1 + getNodeOffset(NodeType::X);
      const uint32_t zx = base1 + getNodeOffset(NodeType::ZX);
      stepg.addFlow({zx, l - 1}, {z0, l});
      stepg.addFlow({zx, l - 1}, {x1, l});
      stepg.mergeProbabilities({zx, l - 1}, p);
    }
  }

  /// @brief Apply a ZY error on qubits (@p q0, @p q1) at layer @p l (L2 only).
  /// @param stepg STEPG to update.
  /// @param q0 First qubit index.
  /// @param q1 Second qubit index.
  /// @param l Time layer index.
  /// @param p Error probability.
  /// @throws std::runtime_error If @p l < 2 at L2.
  HOST static void
  applyZYError(STEPG &stepg, uint32_t q0, uint32_t q1, uint32_t l, double p) {
    if constexpr (Level == CorrelationLevel::L2) {
      if (l < 2) {
        throw std::runtime_error("Mixer: cannot apply ZY error in layer l < 2");
      }
      const uint32_t base0 = q0 * numNodesPerQubit;
      const uint32_t base1 = q1 * numNodesPerQubit;
      const uint32_t z0 = base0 + getNodeOffset(NodeType::Z);
      const uint32_t y1 = base1 + getNodeOffset(NodeType::Y);
      const uint32_t zy = base0 + getNodeOffset(NodeType::ZY);
      stepg.addFlow({zy, l - 2}, {z0, l});
      stepg.addFlow({zy, l - 2}, {y1, l - 1});
      stepg.mergeProbabilities({zy, l - 2}, p);
    }
  }

  /// @brief Apply a ZZ error on qubits (@p q0, @p q1) at layer @p l.
  /// @param stepg STEPG to update.
  /// @param q0 First qubit index.
  /// @param q1 Second qubit index.
  /// @param l Time layer index.
  /// @param p Error probability.
  /// @throws std::runtime_error If @p l < 1.
  HOST static void
  applyZZError(STEPG &stepg, uint32_t q0, uint32_t q1, uint32_t l, double p) {
    if (l < 1) {
      throw std::runtime_error("Mixer: cannot apply ZZ error in layer l < 1");
    }
    const uint32_t base0 = q0 * numNodesPerQubit;
    const uint32_t base1 = q1 * numNodesPerQubit;
    const uint32_t z0 = base0 + getNodeOffset(NodeType::Z);
    const uint32_t z1 = base1 + getNodeOffset(NodeType::Z);
    const uint32_t zz = base1 + getNodeOffset(NodeType::ZZ);
    stepg.addFlow({zz, l - 1}, {z0, l});
    stepg.addFlow({zz, l - 1}, {z1, l});
    stepg.mergeProbabilities({zz, l - 1}, p);
  }
};

} // namespace gp

#endif // GREENPEAS_QEC_ERRORANALYSIS_MIXER_HPP

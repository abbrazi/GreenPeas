#ifndef GREENPEAS_QEC_ERRORANALYSIS_CIRCUIT_HPP
#define GREENPEAS_QEC_ERRORANALYSIS_CIRCUIT_HPP

/// Project headers
#include "GreenPeas/Common.hpp"
#include "GreenPeas/Core/Matrix.hpp"
#include "GreenPeas/Core/Vector.hpp"
#include "GreenPeas/Core/Words.hpp"
#include "GreenPeas/Policies/Storage/Host.hpp"
#include "GreenPeas/QEC/ErrorAnalysis/Mixer.hpp"
#include "GreenPeas/QEC/ErrorAnalysis/STEPG.hpp"

/// Third-party headers
#include "stim.h"

namespace gp {

/// @brief Parameters of a syndrome measurement (SM) circuit.
/// @tparam Level Correlation level.
template <CorrelationLevel Level>
struct CircuitParameters {
  /// @brief Number of qubits in the SM circuit.
  uint32_t numQubits;

  /// @brief Number of layers in the SM circuit.
  uint32_t numLayers;

  /// @brief Number of measurements in the SM circuit.
  uint32_t numMeasurements;

  /// @brief Number of detectors in the SM circuit.
  uint32_t numDetectors;

  /// @brief Number of observables in the SM circuit.
  uint32_t numObservables;

  /// @brief Equality comparison.
  /// @param other Parameters to compare against.
  /// @return True if equal to `other`.
  HOST constexpr auto operator==(const CircuitParameters &other) const -> bool {
    return numQubits == other.numQubits && numLayers == other.numLayers &&
           numMeasurements == other.numMeasurements &&
           numDetectors == other.numDetectors &&
           numObservables == other.numObservables;
  }

  /// @brief Number of error nodes per layer.
  HOST constexpr auto numNodesPerLayer() const -> uint32_t {
    return numQubits * Mixer<Level>::numNodesPerQubit;
  }

  /// @brief Total number of error nodes in the circuit.
  HOST constexpr auto numNodes() const -> uint32_t {
    return numLayers * numNodesPerLayer();
  }

  /// @brief Number of 64-bit sensitivity words per error node.
  HOST constexpr auto numWordsPerNode() const -> uint32_t {
    return numWordsForBits(numDetectors + numObservables);
  }

  /// @brief Build circuit parameters from Stim circuit stats.
  /// @param stats Stim circuit stats.
  /// @return Circuit parameters derived from `stats`.
  HOST static auto fromStimCircuitStats(const stim::CircuitStats &stats)
      -> CircuitParameters {
    CircuitParameters parameters;

    parameters.numQubits = stats.num_qubits;
    parameters.numLayers = stats.num_ticks + 1;
    parameters.numMeasurements = stats.num_measurements;
    parameters.numDetectors = stats.num_detectors;
    parameters.numObservables = stats.num_observables;

    return parameters;
  }
};

/// @brief Non-owning view of a circuit sensitivity matrix.
/// @tparam Layout Layout policy.
template <typename Layout>
using CircuitSensitivityMatrixView = MatrixView<uint32_t, uint64_t, Layout>;

/// @brief Sensitivity matrix for a syndrome measurement (SM) circuit.
///
/// Each row i indicates the detectors/observables sensitive to physical error i
/// in the circuit. Sensitivities are bit-packed into 64-bit words.
///
/// @tparam Storage Storage policy for allocation/copy.
/// @tparam Layout Layout policy.
template <typename Storage, typename Layout>
using CircuitSensitivityMatrix = Matrix<uint32_t, uint64_t, Storage, Layout>;

/// @brief Non-owning view of a compressed sensitivity map.
/// @tparam Layout Layout policy for `vals`.
template <typename Layout>
struct CircuitSensitivityMapView {
  /// @brief Row indices of non-zero entries.
  VectorView<uint32_t, uint32_t> keys;

  /// @brief Packed detector/observable sensitivities for each non-zero row.
  MatrixView<uint32_t, uint64_t, Layout> vals;
};

/// @brief Compressed sensitivity map for a syndrome measurement (SM) circuit.
///
/// Stores only the non-zero rows of the dense sensitivity matrix.
///
/// @tparam Storage Storage policy for allocation/copy.
/// @tparam Layout Layout policy for `vals`.
template <typename Storage, typename Layout>
struct CircuitSensitivityMap {
  /// @brief Row indices of non-zero entries.
  Vector<uint32_t, uint32_t, Storage> keys;

  /// @brief Packed detector/observable sensitivities for each non-zero row.
  Matrix<uint32_t, uint64_t, Storage, Layout> vals;

  /// @brief Construct from unpacked sizes.
  /// @param numMeasurements Number of measurements (rows).
  /// @param numWordsPerNode Number of 64-bit sensitivity words per node (cols).
  HOST CircuitSensitivityMap(uint32_t numMeasurements, uint32_t numWordsPerNode)
      : keys(numMeasurements), vals(numMeasurements, numWordsPerNode) {}

  /// @brief Fit new unpacked sizes.
  /// @param numMeasurements New number of measurements (rows).
  /// @param numWordsPerNode New number of 64-bit sensitivity words per node
  /// (cols).
  HOST void fitto(uint32_t numMeasurements, uint32_t numWordsPerNode) {
    keys.fitto(numMeasurements);
    vals.fitto(numMeasurements, numWordsPerNode);
  }

  /// @brief Copy to another sensitivity map (same shape required).
  /// @tparam DestinationStorage Destination storage policy.
  /// @tparam DestinationLayout Destination layout policy.
  /// @param other Destination map.
  template <typename DestinationStorage, typename DestinationLayout>
  HOST void copyTo(CircuitSensitivityMap<DestinationStorage, DestinationLayout>
                       &other) const {
    keys.copyTo(other.keys);
    vals.copyTo(other.vals);
  }

  /// @brief Copy from another sensitivity map (same shape required).
  /// @tparam SourceStorage Source storage policy.
  /// @tparam SourceLayout Source layout policy.
  /// @param other Source map.
  template <typename SourceStorage, typename SourceLayout>
  HOST void
  copyFrom(const CircuitSensitivityMap<SourceStorage, SourceLayout> &other) {
    keys.copyFrom(other.keys);
    vals.copyFrom(other.vals);
  }

  /// @brief Set all elements in `keys` and `vals`.
  HOST void set() {
    keys.set();
    vals.set();
  }

  /// @brief Clear all elements in `keys` and `vals`.
  HOST void clear() {
    keys.clear();
    vals.clear();
  }

  /// @brief Get a non-owning view of this map.
  /// @return CircuitSensitivityMapView sharing this map's storage.
  HOST auto getView() -> CircuitSensitivityMapView<Layout> {
    return {keys.getView(), vals.getView()};
  }
};

/// @brief Counters of a syndrome measurement (SM) circuit.
struct CircuitCounters {
  /// @brief Layer counter.
  uint32_t layer = 0;

  /// @brief Measurement counter.
  uint32_t measurement = 0;

  /// @brief Detector counter.
  uint32_t detector = 0;

  /// @brief Reset all counters to their defaults.
  HOST void reset() { *this = {}; }
};

/// @brief Syndrome measurement (SM) circuit.
///
/// @tparam Layout Layout policy for initial sensitivity matrix.
/// @tparam Level Correlation level.
template <typename Layout, CorrelationLevel Level>
struct Circuit {
  /// @brief Circuit parameters.
  CircuitParameters<Level> parameters;

  /// @brief Space-time error propagation graph (STEPG).
  STEPG stepg;

  /// @brief Compressed sensitivity map.
  CircuitSensitivityMap<HostStorage, Layout> sensitivityMap;

  /// @brief Layer, measurement, and detector counters.
  CircuitCounters counters;

  /// @brief Construct a circuit from parameters.
  /// @param parameters Circuit parameters.
  HOST explicit Circuit(CircuitParameters<Level> parameters)
      : parameters(parameters),
        stepg(parameters.numLayers, parameters.numNodesPerLayer()),
        sensitivityMap(parameters.numMeasurements,
                       parameters.numWordsPerNode()) {}

  /// @brief Fit new circuit parameters.
  /// @param newParameters New circuit parameters.
  HOST void fitto(CircuitParameters<Level> newParameters) {
    parameters = newParameters;
    stepg.fitto(parameters.numLayers, parameters.numNodesPerLayer());
    sensitivityMap.fitto(parameters.numMeasurements,
                         parameters.numWordsPerNode());
  }

  /// @brief Reset STEPG, sensitivity map, and counters for reuse.
  HOST void reset() {
    stepg.reset();
    sensitivityMap.clear();
    counters.reset();
  }

  /// @brief Apply a gate function to each qubit target.
  /// @tparam ApplyGateFunction Callable `(uint32_t qubit)`.
  /// @param targets Stim gate targets.
  /// @param applyGate Gate function to invoke per target.
  template <typename ApplyGateFunction>
  HOST void applyToEach(stim::SpanRef<const stim::GateTarget> targets,
                        ApplyGateFunction applyGate) {
    for (const auto &target : targets) {
      applyGate(target.qubit_value());
    }
  }

  /// @brief Apply a gate function to each qubit target with a probability.
  /// @tparam ApplyGateFunction Callable `(uint32_t qubit, double probability)`.
  /// @param targets Stim gate targets.
  /// @param probability Noise probability passed to `applyGate`.
  /// @param applyGate Gate function to invoke per target.
  template <typename ApplyGateFunction>
  HOST void applyToEach(stim::SpanRef<const stim::GateTarget> targets,
                        double probability,
                        ApplyGateFunction applyGate) {
    for (const auto &target : targets) {
      applyGate(target.qubit_value(), probability);
    }
  }

  /// @brief Apply a gate function to each target pair.
  /// @tparam ApplyGateFunction Callable `(uint32_t q0, uint32_t q1)`.
  /// @param targets Stim gate targets.
  /// @param applyGate Gate function to invoke per pair.
  template <typename ApplyGateFunction>
  HOST void applyToEachPair(stim::SpanRef<const stim::GateTarget> targets,
                            ApplyGateFunction applyGate) {
    for (size_t i = 0; i < targets.size(); i += 2) {
      applyGate(targets[i].qubit_value(), targets[i + 1].qubit_value());
    }
  }

  /// @brief Apply a gate function to each target pair with a probability.
  /// @tparam ApplyGateFunction Callable `(uint32_t q0, uint32_t q1, double p)`.
  /// @param targets Stim gate targets.
  /// @param probability Noise probability passed to `applyGate`.
  /// @param applyGate Gate function to invoke per pair.
  template <typename ApplyGateFunction>
  HOST void applyToEachPair(stim::SpanRef<const stim::GateTarget> targets,
                            double probability,
                            ApplyGateFunction applyGate) {
    for (size_t i = 0; i < targets.size(); i += 2) {
      applyGate(
          targets[i].qubit_value(), targets[i + 1].qubit_value(), probability);
    }
  }

  /// @brief Apply a Stim DETECTOR instruction.
  HOST auto applyDetector(const stim::CircuitInstruction &op) {
    const uint32_t col = counters.detector / 64;
    const uint32_t bit = counters.detector % 64;

    counters.detector++;

    for (const auto &target : op.targets) {
      const uint32_t row = target.rec_offset() + (int32_t)counters.measurement;

      sensitivityMap.vals(row, col) |= 1ULL << bit;
    }
  }

  /// @brief Apply a Stim OBSERVABLE_INCLUDE instruction.
  HOST void applyObservableInclude(const stim::CircuitInstruction &op) {
    applyDetector(op);
  }

  /// @brief Apply a Stim TICK instruction.
  HOST void applyTick(const stim::CircuitInstruction &op) { counters.layer++; }

  /// @brief Apply Z-basis measurement on qubit @p q with flip probability @p p.
  /// @param q Qubit index.
  /// @param p Measurement flip probability.
  HOST void applyM(uint32_t q, double p) {
    const uint32_t row = Mixer<Level>::applyXError(stepg, q, counters.layer, p);
    sensitivityMap.keys[counters.measurement++] = row;
  }

  /// @brief Apply a Stim M instruction.
  HOST auto applyM(const stim::CircuitInstruction &op) {
    applyToEach(
        op.targets, op.args[0], [this](uint32_t qubit, double probability) {
          this->applyM(qubit, probability);
        });
  }

  /// @brief Apply a Z-basis reset on qubit @p q.
  /// @param q Qubit index.
  HOST void applyR(uint32_t q) {
    Mixer<Level>::applyR(stepg, q, counters.layer);
  }

  /// @brief Apply a Stim R instruction.
  HOST void applyR(const stim::CircuitInstruction &op) {
    applyToEach(op.targets,
                [this](uint32_t qubit) { return this->applyR(qubit); });
  }

  /// @brief Apply a controlled-X gate between @p c and @p t.
  /// @param c Control qubit index.
  /// @param t Target qubit index.
  HOST void applyCX(uint32_t c, uint32_t t) {
    Mixer<Level>::applyCX(stepg, c, t, counters.layer);
  }

  /// @brief Apply a Stim CX instruction.
  HOST void applyCX(const stim::CircuitInstruction &op) {
    applyToEachPair(op.targets, [this](uint32_t control, uint32_t target) {
      return this->applyCX(control, target);
    });
  }

  /// @brief Apply a Hadamard gate on qubit @p q.
  /// @param q Qubit index.
  HOST void applyH(uint32_t q) {
    Mixer<Level>::applyH(stepg, q, counters.layer);
  }

  /// @brief Apply a Stim H instruction.
  HOST void applyH(const stim::CircuitInstruction &op) {
    applyToEach(op.targets,
                [this](uint32_t qubit) { return this->applyH(qubit); });
  }

  /// @brief Apply a single-qubit depolarizing channel on qubit @p q.
  /// @param q Qubit index.
  /// @param p Depolarizing probability.
  HOST void applyDepolarize1(uint32_t q, double p) {
    Mixer<Level>::applyDepolarize1(stepg, q, counters.layer, p);
  }

  /// @brief Apply a Stim DEPOLARIZE1 instruction.
  HOST void applyDepolarize1(const stim::CircuitInstruction &op) {
    applyToEach(
        op.targets, op.args[0], [this](uint32_t qubit, double probability) {
          return this->applyDepolarize1(qubit, probability);
        });
  }

  /// @brief Apply a two-qubit depolarizing channel on qubits @p q0 and @p q1.
  /// @param q0 First qubit index.
  /// @param q1 Second qubit index.
  /// @param p Depolarizing probability.
  HOST void applyDepolarize2(uint32_t q0, uint32_t q1, double p) {
    Mixer<Level>::applyDepolarize2(stepg, q0, q1, counters.layer, p);
  }

  /// @brief Apply a Stim DEPOLARIZE2 instruction.
  HOST void applyDepolarize2(const stim::CircuitInstruction &op) {
    applyToEachPair(
        op.targets,
        op.args[0],
        [this](uint32_t qubit0, uint32_t qubit1, double probability) {
          return this->applyDepolarize2(qubit0, qubit1, probability);
        });
  }

  /// @brief Apply an X error on qubit @p q.
  /// @param q Qubit index.
  /// @param p Error probability.
  HOST void applyXError(uint32_t q, double p) {
    Mixer<Level>::applyXError(stepg, q, counters.layer, p);
  }

  /// @brief Apply a Stim X_ERROR instruction.
  HOST void applyXError(const stim::CircuitInstruction &op) {
    applyToEach(
        op.targets, op.args[0], [this](uint32_t qubit, double probability) {
          return this->applyXError(qubit, probability);
        });
  }

  /// @brief Apply a Stim instruction.
  /// @throws std::invalid_argument If the gate type is unsupported.
  HOST void applyOp(const stim::CircuitInstruction &op) {
    switch (op.gate_type) {
    case stim::GateType::DETECTOR:
      applyDetector(op);
      break;
    case stim::GateType::OBSERVABLE_INCLUDE:
      applyObservableInclude(op);
      break;
    case stim::GateType::TICK:
      applyTick(op);
      break;
    case stim::GateType::M:
      applyM(op);
      break;
    case stim::GateType::R:
      applyR(op);
      break;
    case stim::GateType::CX:
      applyCX(op);
      break;
    case stim::GateType::H:
      applyH(op);
      break;
    case stim::GateType::DEPOLARIZE1:
      applyDepolarize1(op);
      break;
    case stim::GateType::DEPOLARIZE2:
      applyDepolarize2(op);
      break;
    case stim::GateType::X_ERROR:
      applyXError(op);
      break;
    default:
      throw std::invalid_argument(
          "Unsupported operation: " +
          std::string(stim::GATE_DATA[op.gate_type].name));
    }
  }

  /// @brief Initialise the STEPG with default X/Z flows between layers.
  HOST void initialise() {
    Mixer<Level>::initialise(stepg, parameters.numQubits, parameters.numLayers);
  }

  /// @brief Parse a Stim circuit into the circuit's existing buffers.
  /// @param circuit Input Stim circuit (must fit within current capacity).
  HOST void parseFromStimCircuit(const stim::Circuit &circuit) {
    fitto(CircuitParameters<Level>::fromStimCircuitStats(
        circuit.compute_stats()));
    reset();
    initialise();
    circuit.for_each_operation(
        [&](const stim::CircuitInstruction &op) { applyOp(op); });
  }

  /// @brief Build a SM circuit from a Stim circuit.
  /// @param circuit Input Stim circuit.
  /// @return Parsed SM circuit with STEPG and initial sensitivities.
  HOST static auto fromStimCircuit(const stim::Circuit &circuit) -> Circuit {
    Circuit native(CircuitParameters<Level>::fromStimCircuitStats(
        circuit.compute_stats()));
    native.parseFromStimCircuit(circuit);
    return native;
  }
};

} // namespace gp

#endif // GREENPEAS_QEC_ERRORANALYSIS_CIRCUIT_HPP

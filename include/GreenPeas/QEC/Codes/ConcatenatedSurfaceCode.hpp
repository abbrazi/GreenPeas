#ifndef GREENPEAS_QEC_CODES_CONCATENATEDSURFACECODE_HPP
#define GREENPEAS_QEC_CODES_CONCATENATEDSURFACECODE_HPP

/// Standard headers
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

/// Project headers
#include "GreenPeas/QEC/Codes/Code.hpp"

namespace gp {

/// @brief Flag bits for adaptive check selection / corrections.
using Flags = std::vector<uint8_t>;

/// @brief Outer checks supported by each Iceberg block.
using BlockSupport = std::vector<Qubits>;

/// @brief Outer-check measurement strategy for concatenated codes.
enum class MeasurementStrategy : uint32_t {
  /// Measure all outer checks every round.
  Static = 0,
  /// Measure outer checks selected by inner flag diffs.
  Adaptive = 1
};

/// @brief Circuit-level and phenomenological Stim circuit pair.
struct CircuitPair {
  /// @brief Circuit-level noise model.
  stim::Circuit circl;

  /// @brief Phenomenological noise model.
  stim::Circuit pheno;

  /// @brief Append the same instruction to both circuits.
  /// @param gate_name Stim gate name.
  /// @param targets Qubit / record targets.
  /// @param args Optional gate arguments.
  HOST void safe_append_u(std::string_view gate_name,
                          const std::vector<uint32_t> &targets,
                          const std::vector<double> &args = {}) {
    circl.safe_append_u(gate_name, targets, args);
    pheno.safe_append_u(gate_name, targets, args);
  }

  /// @brief Concatenate matching noise-model circuits.
  HOST auto operator+(const CircuitPair &other) const -> CircuitPair {
    return {circl + other.circl, pheno + other.pheno};
  }

  /// @brief Concatenate matching noise-model circuits in place.
  HOST auto operator+=(const CircuitPair &other) -> CircuitPair & {
    circl += other.circl;
    pheno += other.pheno;
    return *this;
  }
};

/// @brief Qubit IDs for a concatenated Iceberg + surface-code layout.
struct ConcatenatedQubitIDs {
  /// @brief All qubit indices.
  Qubits all;

  /// @brief Physical data qubit indices.
  Qubits data;

  /// @brief All check ancilla indices.
  Qubits checks;

  /// @brief Inner Iceberg X-check ancillas.
  Qubits innerXChecks;

  /// @brief Inner Iceberg Z-check ancillas.
  Qubits innerZChecks;

  /// @brief All inner Iceberg check ancillas.
  Qubits innerChecks;

  /// @brief Outer surface-code X-check ancillas.
  Qubits outerXChecks;

  /// @brief Outer surface-code Z-check ancillas.
  Qubits outerZChecks;

  /// @brief All outer surface-code check ancillas.
  Qubits outerChecks;
};

/// @brief Get qubit IDs for a concatenated Iceberg + surface-code layout.
/// @param numData Number of physical data qubits.
/// @param numInnerXChecks Number of inner Iceberg X checks.
/// @param numInnerZChecks Number of inner Iceberg Z checks.
/// @param numOuterXChecks Number of outer surface-code X checks.
/// @param numOuterZChecks Number of outer surface-code Z checks.
/// @return Contiguous concatenated qubit ID layout.
HOST inline auto getConcatenatedQubitIds(uint32_t numData,
                                         uint32_t numInnerXChecks,
                                         uint32_t numInnerZChecks,
                                         uint32_t numOuterXChecks,
                                         uint32_t numOuterZChecks)
    -> ConcatenatedQubitIDs {
  const uint32_t numInnerChecks = numInnerXChecks + numInnerZChecks;
  const uint32_t numOuterChecks = numOuterXChecks + numOuterZChecks;
  const uint32_t numChecks = numInnerChecks + numOuterChecks;
  const uint32_t numAll = numData + numChecks;

  ConcatenatedQubitIDs ids;
  ids.all.resize(numAll);
  std::iota(ids.all.begin(), ids.all.end(), 0);

  ids.data.resize(numData);
  std::iota(ids.data.begin(), ids.data.end(), 0);

  ids.checks.resize(numChecks);
  std::iota(ids.checks.begin(), ids.checks.end(), numData);

  ids.innerXChecks.resize(numInnerXChecks);
  std::iota(ids.innerXChecks.begin(), ids.innerXChecks.end(), numData);

  ids.innerZChecks.resize(numInnerZChecks);
  std::iota(ids.innerZChecks.begin(),
            ids.innerZChecks.end(),
            numData + numInnerXChecks);

  ids.innerChecks.reserve(numInnerChecks);
  ids.innerChecks.insert(
      ids.innerChecks.end(), ids.innerXChecks.begin(), ids.innerXChecks.end());
  ids.innerChecks.insert(
      ids.innerChecks.end(), ids.innerZChecks.begin(), ids.innerZChecks.end());

  ids.outerXChecks.resize(numOuterXChecks);
  std::iota(ids.outerXChecks.begin(),
            ids.outerXChecks.end(),
            numData + numInnerChecks);

  ids.outerZChecks.resize(numOuterZChecks);
  std::iota(ids.outerZChecks.begin(),
            ids.outerZChecks.end(),
            numData + numInnerChecks + numOuterXChecks);

  ids.outerChecks.reserve(numOuterChecks);
  ids.outerChecks.insert(
      ids.outerChecks.end(), ids.outerXChecks.begin(), ids.outerXChecks.end());
  ids.outerChecks.insert(
      ids.outerChecks.end(), ids.outerZChecks.begin(), ids.outerZChecks.end());

  return ids;
}

/// @brief Append noisy CNOT layers for pairs involving flagged qubits.
/// @param circuits Circuits to extend.
/// @param schedule Layered CNOT schedule.
/// @param flagged Lookup of flagged qubit IDs.
/// @param p Physical error rate.
/// @param all Full list of qubit indices (must be sorted).
/// @param idle Reusable buffer for idle qubits each layer.
HOST inline void appendFlaggedCNOTLayers(CircuitPair &circuits,
                                         const CNOTSchedule &schedule,
                                         const Flags &flagged,
                                         double p,
                                         const Qubits &all,
                                         Qubits &idle) {
  for (const auto &active : schedule) {
    Qubits flaggedActive;
    flaggedActive.reserve(active.size());
    for (uint32_t i = 0; i + 1 < active.size(); i += 2) {
      const uint32_t q0 = active[i];
      const uint32_t q1 = active[i + 1];
      if ((q0 < flagged.size() && flagged[q0]) ||
          (q1 < flagged.size() && flagged[q1])) {
        flaggedActive.push_back(q0);
        flaggedActive.push_back(q1);
      }
    }
    if (flaggedActive.empty()) {
      continue;
    }
    circuits.safe_append_u("CNOT", flaggedActive);
    circuits.safe_append_u("TICK", {});
    circuits.circl.safe_append_u("DEPOLARIZE2", flaggedActive, {p});
    getIdleQubits(all, flaggedActive, idle);
    if (!idle.empty()) {
      circuits.circl.safe_append_u("DEPOLARIZE1", idle, {p / 10});
    }
  }
}

/// @brief Append CNOT layers for pairs.
/// @param circuits Circuits to extend.
/// @param schedule Layered CNOT schedule.
/// @param p Physical error rate.
/// @param all Full list of qubit indices (must be sorted).
/// @param idle Reusable buffer for idle qubits each layer.
HOST inline void appendCNOTLayers(CircuitPair &circuits,
                                  const CNOTSchedule &schedule,
                                  double p,
                                  const Qubits &all,
                                  Qubits &idle) {
  for (const auto &active : schedule) {
    circuits.safe_append_u("CNOT", active);
    circuits.safe_append_u("TICK", {});
    circuits.circl.safe_append_u("DEPOLARIZE2", active, {p});
    getIdleQubits(all, active, idle);
    if (!idle.empty()) {
      circuits.circl.safe_append_u("DEPOLARIZE1", idle, {p / 10});
    }
  }
}

/// @brief This is the most theoretically difficult part of the codebase!
///
/// Please read Berthusen et al. [1] before continuing.
///
/// The first mountain is code concatenation. Take an inner Iceberg code:
///
///   2 -- 3
///   |    |
///   0 -- 1
///
/// The stabilisers are
///
///   - SX = X0 X1 X2 X3
///   - SZ = Z0 Z1 Z2 Z3
///
/// Each Iceberg code encodes k = 2 logical qubits, defined by:
///
///   - XL0 = X1 X2
///   - XL1 = X1 X3
///   - ZL0 = Z2 Z4
///   - ZL1 = Z3 Z4
///
/// We need to assign each of the n data qubits in the outer [[n, k, d]] code to
/// either L0 or L1 in one of n / 2 Iceberg code blocks. It's critical that this
/// assignment satisfies the following constraints:
///
/// (a) The data qubits supported by a block must be check-disjoint.
///     This is required to avoid sub-optimal distances.
/// (b) The data qubits supported by a block must be color-disjoint
///     This is required to avoid scheduling conflicts.
///
/// This problem can be solved using graph matching. It can also be solved using
/// a heuristic derived from some assumed layout of the outer code. For example,
/// consider a distance 4 rotated planar surface code:
///
///      12   13   14   15
///    Z    X    Z    X    Z
///       8    9   10   11
///    X    Z    X    Z    X
///       4    5    6    7
///    Z    X    Z    X    Z
///       0    1    2    3
///
/// Note: assume standard N-shaped schedules for X and Z-shaped schedules for Z.
///
/// For each unassigned data qubit at coordinates (r, c), pair it with:
///
///   (r', c') = (d - 1 - r, (c + 2) mod d)
///
/// i.e., a vertical reflection plus a 2-column cycle shift.
///
/// Pairs = {(0, 14), (1, 15), (2, 12), ...}
///
/// [1] https://arxiv.org/abs/2502.14835
struct ConcatenatedSurfaceCode {
  /// @brief Outer X-parity-check matrix (values are schedule layers).
  PCM Hx;

  /// @brief Outer Z-parity-check matrix (values are schedule layers).
  PCM Hz;

  /// @brief Outer X-logical-check matrix.
  LCM Lx;

  /// @brief Outer Z-logical-check matrix.
  LCM Lz;

  /// @brief Stim tableau simulator used for adaptive flag tracking.
  stim::TableauSimulator<64> sim;

  /// @brief Qubit index layout.
  ConcatenatedQubitIDs qubitIDs;

  /// @brief Layered CNOT schedule for inner X checks.
  CNOTSchedule innerCheckXSchedule;

  /// @brief Layered CNOT schedule for inner Z checks.
  CNOTSchedule innerCheckZSchedule;

  /// @brief Layered CNOT schedule for outer X checks.
  CNOTSchedule outerCheckXSchedule;

  /// @brief Layered CNOT schedule for outer Z checks.
  CNOTSchedule outerCheckZSchedule;

  /// @brief Interleaved outer X/Z CNOT schedule.
  CNOTSchedule outerInterleavedSchedule;

  /// @brief Physical data qubits per outer X check.
  CheckSupport outerCheckXSupport;

  /// @brief Physical data qubits per outer Z check.
  CheckSupport outerCheckZSupport;

  /// @brief Outer X-check ancillas per Iceberg block.
  BlockSupport blockXSupport;

  /// @brief Outer Z-check ancillas per Iceberg block.
  BlockSupport blockZSupport;

  /// @brief Physical data qubits per outer X logical.
  LogicalSupport outerLogicalXSupport;

  /// @brief Physical data qubits per outer Z logical.
  LogicalSupport outerLogicalZSupport;

  /// @brief Pending inner X corrections from flagged Iceberg X checks.
  Flags innerXCorrections;

  /// @brief Pending inner Z corrections from flagged Iceberg Z checks.
  Flags innerZCorrections;

  /// @brief Construct a concatenated surface code with outer distance `d`.
  /// @param d Outer surface-code distance.
  /// @param root Root data directory.
  HOST explicit ConcatenatedSurfaceCode(uint32_t d, const std::string &root)
      : Hx(PCM::fromMtx(getCodePath(CodeType::Surface, d, root) + "/Hx.mtx")),
        Hz(PCM::fromMtx(getCodePath(CodeType::Surface, d, root) + "/Hz.mtx")),
        Lx(LCM::fromMtx(getCodePath(CodeType::Surface, d, root) + "/Lx.mtx")),
        Lz(LCM::fromMtx(getCodePath(CodeType::Surface, d, root) + "/Lz.mtx")),
        sim(stim::externally_seeded_rng()) {
    init();
  }

  /// @brief Initialise qubit IDs, schedules, and supports from loaded matrices.
  HOST void init() {
    const uint32_t numOuterData = Hx.dimensions.numCols;
    const uint32_t numInnerXChecks = numOuterData / 2;
    const uint32_t numInnerZChecks = Hz.dimensions.numCols / 2;
    const uint32_t numOuterXChecks = Hx.dimensions.numRows;
    const uint32_t numOuterZChecks = Hz.dimensions.numRows;
    const uint32_t numData = 2 * numOuterData;

    qubitIDs = getConcatenatedQubitIds(numData,
                                       numInnerXChecks,
                                       numInnerZChecks,
                                       numOuterXChecks,
                                       numOuterZChecks);

    innerXCorrections.assign(numInnerXChecks, 0);
    innerZCorrections.assign(numInnerZChecks, 0);

    // Pair outer data qubits into Iceberg blocks (square/even-d heuristic).
    const uint32_t d = static_cast<uint32_t>(std::sqrt(numOuterData));
    std::vector<std::pair<uint32_t, uint8_t>> blocks(numOuterData);
    std::vector<uint8_t> assigned(numOuterData, 0);
    uint32_t block = 0;
    for (uint32_t r = 0; r < d; ++r) {
      for (uint32_t c = 0; c < d; ++c) {
        const uint32_t q0 = r * d + c;
        if (assigned[q0]) {
          continue;
        }
        const uint32_t rp = (d - 1) - r;
        const uint32_t cp = (c + 2) % d;
        const uint32_t q1 = rp * d + cp;
        blocks[q0] = {block, 0};
        blocks[q1] = {block, 1};
        assigned[q0] = 1;
        assigned[q1] = 1;
        ++block;
      }
    }

    // Build the 4-layer CNOT schedule for each inner Iceberg X check.
    innerCheckXSchedule.assign(4, {});
    for (uint32_t i = 0; i < numInnerXChecks; ++i) {
      const uint32_t base = i * 4;
      const uint32_t ancilla = qubitIDs.innerXChecks[i];
      innerCheckXSchedule[0].push_back(ancilla);
      innerCheckXSchedule[0].push_back(base + 0);
      innerCheckXSchedule[1].push_back(ancilla);
      innerCheckXSchedule[1].push_back(base + 1);
      innerCheckXSchedule[2].push_back(ancilla);
      innerCheckXSchedule[2].push_back(base + 2);
      innerCheckXSchedule[3].push_back(ancilla);
      innerCheckXSchedule[3].push_back(base + 3);
    }

    // Build the 4-layer CNOT schedule for each inner Iceberg Z check.
    innerCheckZSchedule.assign(4, {});
    for (uint32_t i = 0; i < numInnerZChecks; ++i) {
      const uint32_t base = i * 4;
      const uint32_t ancilla = qubitIDs.innerZChecks[i];
      innerCheckZSchedule[0].push_back(base + 0);
      innerCheckZSchedule[0].push_back(ancilla);
      innerCheckZSchedule[1].push_back(base + 1);
      innerCheckZSchedule[1].push_back(ancilla);
      innerCheckZSchedule[2].push_back(base + 2);
      innerCheckZSchedule[2].push_back(ancilla);
      innerCheckZSchedule[3].push_back(base + 3);
      innerCheckZSchedule[3].push_back(ancilla);
    }

    // Lift outer X checks onto physical qubits and expand their schedule.
    outerCheckXSupport.assign(numOuterXChecks, {});
    blockXSupport.assign(numInnerXChecks, {});
    for (uint32_t i = 0; i < Hx.dimensions.numNonZeros; ++i) {
      const uint32_t row = Hx.rows[i];
      const uint32_t col = Hx.cols[i];
      const uint32_t layer = Hx.vals[i];
      const uint32_t phy = 2 * layer;
      const auto &[blockId, logical] = blocks[col];
      const uint32_t base = 4 * blockId;
      const uint32_t q0 = base;
      const uint32_t q1 = base + 1 + logical;
      const uint32_t ancilla = qubitIDs.outerXChecks[row];

      outerCheckXSupport[row].push_back(q0);
      outerCheckXSupport[row].push_back(q1);
      blockXSupport[blockId].push_back(ancilla);

      if (phy + 1 >= outerCheckXSchedule.size()) {
        outerCheckXSchedule.resize(phy + 2);
      }
      outerCheckXSchedule[phy + 0].push_back(ancilla);
      outerCheckXSchedule[phy + 0].push_back(q0);
      outerCheckXSchedule[phy + 1].push_back(ancilla);
      outerCheckXSchedule[phy + 1].push_back(q1);
    }

    // Lift outer X logicals onto physical qubits.
    for (uint32_t i = 0; i < Lx.dimensions.numNonZeros; ++i) {
      const uint32_t row = Lx.rows[i];
      const uint32_t col = Lx.cols[i];
      const auto &[blockId, logical] = blocks[col];
      const uint32_t base = 4 * blockId;
      outerLogicalXSupport[row].push_back(base);
      outerLogicalXSupport[row].push_back(base + 1 + logical);
    }

    // Lift outer Z checks onto physical qubits and expand their schedule.
    outerCheckZSupport.assign(numOuterZChecks, {});
    blockZSupport.assign(numInnerZChecks, {});
    for (uint32_t i = 0; i < Hz.dimensions.numNonZeros; ++i) {
      const uint32_t row = Hz.rows[i];
      const uint32_t col = Hz.cols[i];
      const uint32_t layer = Hz.vals[i];
      const uint32_t phy = 2 * layer;
      const auto &[blockId, logical] = blocks[col];
      const uint32_t base = 4 * blockId;
      const uint32_t q0 = base + 3;
      const uint32_t q1 = base + 1 + logical;
      const uint32_t ancilla = qubitIDs.outerZChecks[row];

      outerCheckZSupport[row].push_back(q0);
      outerCheckZSupport[row].push_back(q1);
      blockZSupport[blockId].push_back(ancilla);

      if (phy + 1 >= outerCheckZSchedule.size()) {
        outerCheckZSchedule.resize(phy + 2);
      }
      outerCheckZSchedule[phy + 0].push_back(q0);
      outerCheckZSchedule[phy + 0].push_back(ancilla);
      outerCheckZSchedule[phy + 1].push_back(q1);
      outerCheckZSchedule[phy + 1].push_back(ancilla);
    }

    // Lift outer Z logicals onto physical qubits.
    for (uint32_t i = 0; i < Lz.dimensions.numNonZeros; ++i) {
      const uint32_t row = Lz.rows[i];
      const uint32_t col = Lz.cols[i];
      const auto &[blockId, logical] = blocks[col];
      const uint32_t base = 4 * blockId;
      outerLogicalZSupport[row].push_back(base + 3);
      outerLogicalZSupport[row].push_back(base + 1 + logical);
    }

    // Interleave outer X and Z CNOT layers into one schedule.
    const auto numLayers =
        std::max(outerCheckXSchedule.size(), outerCheckZSchedule.size());
    outerInterleavedSchedule.assign(numLayers, {});
    for (auto i = 0; i < numLayers; ++i) {
      if (i < outerCheckXSchedule.size()) {
        outerInterleavedSchedule[i].insert(outerInterleavedSchedule[i].end(),
                                           outerCheckXSchedule[i].begin(),
                                           outerCheckXSchedule[i].end());
      }
      if (i < outerCheckZSchedule.size()) {
        outerInterleavedSchedule[i].insert(outerInterleavedSchedule[i].end(),
                                           outerCheckZSchedule[i].begin(),
                                           outerCheckZSchedule[i].end());
      }
    }
  }

  /// @brief Get the head of the circuits.
  /// @param p Physical error rate.
  /// @return State-preparation fragments.
  HOST auto getHead(double p) -> CircuitPair {
    CircuitPair circuits;

    circuits.safe_append_u("R", qubitIDs.data);
    circuits.safe_append_u("TICK", {});
    circuits.circl.safe_append_u("X_ERROR", qubitIDs.data, {p});
    circuits.circl.safe_append_u("DEPOLARIZE1", qubitIDs.checks, {p / 10});

    sim.safe_do_circuit(circuits.circl);

    return circuits;
  }

  /// @brief Get one inner Iceberg QED round.
  /// @param p Physical error rate.
  /// @return Inner QED fragments.
  HOST auto getQEDRound(double p) const -> CircuitPair {
    CircuitPair circuits;

    Qubits idle;
    idle.reserve(qubitIDs.all.size());

    circuits.safe_append_u("R", qubitIDs.innerChecks);
    circuits.safe_append_u("TICK", {});
    circuits.circl.safe_append_u("X_ERROR", qubitIDs.innerChecks, {p});
    getIdleQubits(qubitIDs.all, qubitIDs.innerChecks, idle);
    if (!idle.empty()) {
      circuits.circl.safe_append_u("DEPOLARIZE1", idle, {p / 10});
    }
    circuits.pheno.safe_append_u("DEPOLARIZE1", qubitIDs.data, {p / 10});

    if (!qubitIDs.innerXChecks.empty()) {
      circuits.safe_append_u("H", qubitIDs.innerXChecks);
      circuits.safe_append_u("TICK", {});
      circuits.circl.safe_append_u("DEPOLARIZE1", qubitIDs.all, {p / 10});
    }

    appendCNOTLayers(circuits, innerCheckXSchedule, p, qubitIDs.all, idle);
    appendCNOTLayers(circuits, innerCheckZSchedule, p, qubitIDs.all, idle);

    if (!qubitIDs.innerXChecks.empty()) {
      circuits.safe_append_u("H", qubitIDs.innerXChecks);
      circuits.safe_append_u("TICK", {});
      circuits.circl.safe_append_u("DEPOLARIZE1", qubitIDs.all, {p / 10});
    }

    circuits.safe_append_u("M", qubitIDs.innerChecks, {p});
    return circuits;
  }

  /// @brief Get one outer QEC round over flagged checks.
  /// @param p Physical error rate.
  /// @param flags Per-outer-check flag bits (X then Z).
  /// @return Outer QEC fragments.
  HOST auto getQECRound(double p, const Flags &flags) const -> CircuitPair {
    CircuitPair circuits;

    const uint32_t numData = (uint32_t)qubitIDs.data.size();
    const uint32_t numInnerChecks = (uint32_t)qubitIDs.innerChecks.size();
    const uint32_t numOuterXChecks = (uint32_t)qubitIDs.outerXChecks.size();
    const uint32_t numOuterChecks = (uint32_t)qubitIDs.outerChecks.size();

    Qubits flaggedX;
    flaggedX.reserve(numOuterXChecks);
    for (uint32_t i = 0; i < numOuterXChecks; ++i) {
      if (flags[i]) {
        flaggedX.push_back(qubitIDs.outerXChecks[i]);
      }
    }

    Qubits flaggedZ;
    flaggedZ.reserve(numOuterChecks - numOuterXChecks);
    for (uint32_t i = numOuterXChecks; i < numOuterChecks; ++i) {
      if (flags[i]) {
        flaggedZ.push_back(qubitIDs.outerZChecks[i - numOuterXChecks]);
      }
    }

    Qubits flagged;
    flagged.reserve(flaggedX.size() + flaggedZ.size());
    flagged.insert(flagged.end(), flaggedX.begin(), flaggedX.end());
    flagged.insert(flagged.end(), flaggedZ.begin(), flaggedZ.end());

    if (flagged.empty()) {
      return circuits;
    }

    Qubits idle;
    idle.reserve(qubitIDs.all.size());

    circuits.safe_append_u("R", flagged);
    circuits.safe_append_u("TICK", {});
    circuits.circl.safe_append_u("X_ERROR", flagged, {p});
    getIdleQubits(qubitIDs.all, flagged, idle);
    if (!idle.empty()) {
      circuits.circl.safe_append_u("DEPOLARIZE1", idle, {p / 10});
    }
    circuits.pheno.safe_append_u("DEPOLARIZE1", qubitIDs.data, {p / 10});

    if (!flaggedX.empty()) {
      circuits.safe_append_u("H", flaggedX);
      circuits.safe_append_u("TICK", {});
      circuits.circl.safe_append_u("DEPOLARIZE1", qubitIDs.all, {p / 10});
    }

    const bool allOuter = flagged.size() == numOuterChecks;
    if (allOuter) {
      appendCNOTLayers(
          circuits, outerInterleavedSchedule, p, qubitIDs.all, idle);
    } else {
      Flags flaggedLookup(numData + numInnerChecks + numOuterChecks, 0);
      for (const uint32_t q : flagged) {
        flaggedLookup[q] = 1;
      }
      appendFlaggedCNOTLayers(circuits,
                              outerInterleavedSchedule,
                              flaggedLookup,
                              p,
                              qubitIDs.all,
                              idle);
    }

    if (!flaggedX.empty()) {
      circuits.safe_append_u("H", flaggedX);
      circuits.safe_append_u("TICK", {});
      circuits.circl.safe_append_u("DEPOLARIZE1", qubitIDs.all, {p / 10});
    }

    circuits.safe_append_u("M", flagged, {p});
    return circuits;
  }

  /// @brief Element-wise XOR of two flag vectors.
  /// @param a First flag vector.
  /// @param b Second flag vector.
  /// @return Element-wise `a XOR b` (truncated to `a.size()`).
  HOST static auto xorFlags(const Flags &a, const Flags &b) -> Flags {
    const size_t n = a.size();
    Flags out(n, 0);
    for (size_t i = 0; i < n && i < b.size(); ++i) {
      out[i] = a[i] ^ b[i];
    }
    return out;
  }

  /// @brief Read inner Iceberg measurement flags from the simulator record.
  /// @return Per-inner-check flag bits (X then Z).
  HOST auto getInnerFlags() -> Flags {
    const uint32_t numInnerXChecks = (uint32_t)qubitIDs.innerXChecks.size();
    const uint32_t numInnerZChecks = (uint32_t)qubitIDs.innerZChecks.size();
    const uint32_t numInnerChecks = (uint32_t)qubitIDs.innerChecks.size();

    Flags flags(numInnerChecks, 0);
    for (uint32_t i = 0; i < numInnerXChecks; ++i) {
      if (sim.measurement_record.lookback(numInnerChecks - i)) {
        flags[i] = 1;
      }
    }
    for (uint32_t i = 0; i < numInnerZChecks; ++i) {
      if (sim.measurement_record.lookback(numInnerZChecks - i)) {
        flags[numInnerXChecks + i] = 1;
      }
    }
    return flags;
  }

  /// @brief Map inner flag diffs onto outer checks that share an Iceberg block.
  /// @param diff XOR of consecutive inner flag vectors.
  /// @return Per-outer-check flag bits (X then Z).
  HOST auto getOuterFlags(const Flags &diff) -> Flags {
    const uint32_t numData = (uint32_t)qubitIDs.data.size();
    const uint32_t numInnerXChecks = (uint32_t)qubitIDs.innerXChecks.size();
    const uint32_t numInnerZChecks = (uint32_t)qubitIDs.innerZChecks.size();
    const uint32_t numInnerChecks = (uint32_t)qubitIDs.innerChecks.size();
    const uint32_t numOuterChecks = (uint32_t)qubitIDs.outerChecks.size();

    Flags flags(numOuterChecks, 0);
    for (uint32_t i = 0; i < numInnerXChecks; ++i) {
      if (!diff[i]) {
        continue;
      }
      for (const uint32_t q : blockXSupport[i]) {
        flags[q - numData - numInnerChecks] = 1;
      }
    }
    for (uint32_t i = 0; i < numInnerZChecks; ++i) {
      if (!diff[numInnerXChecks + i]) {
        continue;
      }
      for (const uint32_t q : blockZSupport[i]) {
        flags[q - numData - numInnerChecks] = 1;
      }
    }
    return flags;
  }

  /// @brief Accumulate pending inner corrections from flagged Iceberg checks.
  /// @param innerFlags Per-inner-check flag bits (X then Z).
  HOST void trackInnerCorrections(const Flags &innerFlags) {
    const uint32_t numInnerXChecks = (uint32_t)qubitIDs.innerXChecks.size();
    const uint32_t numInnerZChecks = (uint32_t)qubitIDs.innerZChecks.size();

    for (uint32_t i = 0; i < numInnerXChecks; ++i) {
      if (innerFlags[i]) {
        innerXCorrections[i] ^= 1;
      }
    }
    for (uint32_t i = 0; i < numInnerZChecks; ++i) {
      if (innerFlags[numInnerXChecks + i]) {
        innerZCorrections[i] ^= 1;
      }
    }
  }

  /// @brief Apply accumulated inner X corrections to the final data record.
  HOST void applyInnerCorrections() {
    const uint32_t numData = (uint32_t)qubitIDs.data.size();
    const uint32_t numInnerXChecks = (uint32_t)qubitIDs.innerXChecks.size();
    auto dataRecord = sim.measurement_record.storage.end() - numData;
    for (uint32_t i = 0; i < numInnerXChecks; ++i) {
      if (!innerXCorrections[i]) {
        continue;
      }
      const uint32_t q = 4 * i;
      dataRecord[q] = !dataRecord[q];
    }
  }

  /// @brief Get `rounds` QED/QEC cycles with detector annotations.
  /// @param rounds Number of QEC rounds.
  /// @param p Physical error rate.
  /// @param strategy Outer-check measurement strategy.
  /// @param includeXDetectors Include X-check detectors each round.
  /// @return Detector-annotated QED/QEC cycles.
  HOST auto getCycle(uint32_t rounds,
                     double p,
                     MeasurementStrategy strategy = MeasurementStrategy::Static,
                     bool includeXDetectors = true) -> CircuitPair {
    const uint32_t numInnerXChecks = (uint32_t)qubitIDs.innerXChecks.size();
    const uint32_t numInnerZChecks = (uint32_t)qubitIDs.innerZChecks.size();
    const uint32_t numInnerChecks = (uint32_t)qubitIDs.innerChecks.size();
    const uint32_t numOuterXChecks = (uint32_t)qubitIDs.outerXChecks.size();
    const uint32_t numOuterZChecks = (uint32_t)qubitIDs.outerZChecks.size();
    const uint32_t numOuterChecks = (uint32_t)qubitIDs.outerChecks.size();

    Flags prevInnerFlags(numInnerChecks, 0);
    Flags currInnerFlags(numInnerChecks, 0);
    Flags diffInnerFlags(numInnerChecks, 0);
    Flags outerFlags(numOuterChecks, 1);

    uint32_t totalMeasurements = numInnerChecks + numOuterChecks;
    std::vector<uint32_t> lastOuterMeasurement(numOuterChecks);
    for (uint32_t i = 0; i < numOuterChecks; ++i) {
      lastOuterMeasurement[i] = numInnerChecks + i;
    }

    uint32_t round = 0;

    auto qed = getQEDRound(p);
    for (uint32_t i = 0; i < numInnerZChecks; ++i) {
      const auto targets =
          Qubits{(numInnerZChecks - i) | stim::TARGET_RECORD_BIT};
      const std::vector<double> args = {
          1.0, (double)qubitIDs.innerZChecks[i], (double)round};
      qed.safe_append_u("DETECTOR", targets, args);
    }
    sim.safe_do_circuit(qed.circl);

    prevInnerFlags = getInnerFlags();
    trackInnerCorrections(prevInnerFlags);

    auto qec = getQECRound(p, outerFlags);
    for (uint32_t i = 0; i < numOuterZChecks; ++i) {
      const auto targets =
          Qubits{(numOuterZChecks - i) | stim::TARGET_RECORD_BIT};
      const std::vector<double> args = {
          1.0, (double)qubitIDs.outerZChecks[i], (double)round};
      qec.safe_append_u("DETECTOR", targets, args);
    }
    sim.safe_do_circuit(qec.circl);

    const auto init = qed + qec;
    ++round;

    CircuitPair mid;
    uint32_t numFlagged = numOuterChecks;

    for (; round < rounds; ++round) {
      qed = getQEDRound(p);

      if (includeXDetectors) {
        for (uint32_t i = 0; i < numInnerXChecks; ++i) {
          const auto targets = Qubits{
              (uint32_t)(numInnerChecks * 1 - i) | stim::TARGET_RECORD_BIT,
              (uint32_t)(numInnerChecks * 2 - i + numFlagged) |
                  stim::TARGET_RECORD_BIT};
          const std::vector<double> args = {
              1.0, (double)qubitIDs.innerChecks[i], (double)round};
          qed.safe_append_u("DETECTOR", targets, args);
        }
      }
      for (uint32_t i = numInnerXChecks; i < numInnerChecks; ++i) {
        const auto targets =
            Qubits{(uint32_t)(numInnerChecks * 1 - i) | stim::TARGET_RECORD_BIT,
                   (uint32_t)(numInnerChecks * 2 - i + numFlagged) |
                       stim::TARGET_RECORD_BIT};
        const std::vector<double> args = {
            1.0, (double)qubitIDs.innerChecks[i], (double)round};
        qed.safe_append_u("DETECTOR", targets, args);
      }
      sim.safe_do_circuit(qed.circl);

      currInnerFlags = getInnerFlags();
      trackInnerCorrections(currInnerFlags);

      if (strategy == MeasurementStrategy::Static) {
        outerFlags.assign(numOuterChecks, 1);
      } else {
        diffInnerFlags = xorFlags(prevInnerFlags, currInnerFlags);
        prevInnerFlags = currInnerFlags;
        outerFlags = getOuterFlags(diffInnerFlags);

        // Measure all outer checks at the end and halfway through.
        if (round == rounds - 1 || round == rounds / 2) {
          outerFlags.assign(numOuterChecks, 1);
        }
      }

      qec = getQECRound(p, outerFlags);

      numFlagged =
          (uint32_t)std::count(outerFlags.begin(), outerFlags.end(), 1);
      totalMeasurements += numInnerChecks + numFlagged;

      uint32_t k = 0;
      for (uint32_t i = 0; i < numOuterChecks; ++i) {
        const uint32_t prevMeasurement = lastOuterMeasurement[i];
        uint32_t currMeasurement = lastOuterMeasurement[i];

        if (outerFlags[i]) {
          currMeasurement = totalMeasurements - numFlagged + k;
          lastOuterMeasurement[i] = currMeasurement;
          ++k;
        }

        const uint32_t delta = currMeasurement - prevMeasurement;
        if (delta > 0 && (includeXDetectors || i >= numOuterXChecks)) {
          const auto targets = Qubits{
              (totalMeasurements - prevMeasurement) | stim::TARGET_RECORD_BIT,
              (totalMeasurements - currMeasurement) | stim::TARGET_RECORD_BIT};
          const std::vector<double> args = {
              1.0, (double)qubitIDs.outerChecks[i], (double)round};
          qec.safe_append_u("DETECTOR", targets, args);
        }
      }
      sim.safe_do_circuit(qec.circl);

      mid += qed + qec;
    }

    return init + mid;
  }

  /// @brief Get the tail of the circuit.
  /// @param rounds Round index used for detector coordinates.
  /// @param p Physical error rate.
  /// @return Final data-measurement fragments.
  HOST auto getTail(uint32_t rounds, double p) -> CircuitPair {
    CircuitPair circuits;
    const uint32_t numData = (uint32_t)qubitIDs.data.size();
    const uint32_t numInnerZChecks = (uint32_t)qubitIDs.innerZChecks.size();
    const uint32_t numOuterZChecks = (uint32_t)qubitIDs.outerZChecks.size();
    const uint32_t numOuterChecks = (uint32_t)qubitIDs.outerChecks.size();

    // Tail noise is measurement flips only, so circl and pheno match.
    circuits.safe_append_u("M", qubitIDs.data, {p});

    for (uint32_t i = 0; i < numInnerZChecks; ++i) {
      const uint32_t base = 4 * i;
      Qubits records;
      records.reserve(5);
      records.push_back((numData - base - 0) | stim::TARGET_RECORD_BIT);
      records.push_back((numData - base - 1) | stim::TARGET_RECORD_BIT);
      records.push_back((numData - base - 2) | stim::TARGET_RECORD_BIT);
      records.push_back((numData - base - 3) | stim::TARGET_RECORD_BIT);
      records.push_back((numData + numOuterChecks + numInnerZChecks - i) |
                        stim::TARGET_RECORD_BIT);
      circuits.safe_append_u(
          "DETECTOR",
          records,
          {1.0, (double)qubitIDs.innerZChecks[i], (double)rounds});
    }

    for (uint32_t check = 0; check < numOuterZChecks; ++check) {
      const auto &support = outerCheckZSupport[check];
      if (support.empty()) {
        continue;
      }
      Qubits records;
      records.reserve(support.size() + 1);
      for (const uint32_t q : support) {
        records.push_back((numData - q) | stim::TARGET_RECORD_BIT);
      }
      records.push_back((numData + numOuterZChecks - check) |
                        stim::TARGET_RECORD_BIT);
      circuits.safe_append_u(
          "DETECTOR",
          records,
          {1.0, (double)qubitIDs.outerZChecks[check], (double)rounds});
    }

    for (const auto &[logical, support] : outerLogicalZSupport) {
      Qubits records;
      records.reserve(support.size());
      for (const uint32_t q : support) {
        records.push_back((numData - q) | stim::TARGET_RECORD_BIT);
      }
      circuits.safe_append_u("OBSERVABLE_INCLUDE", records, {(double)logical});
    }

    sim.safe_do_circuit(circuits.circl);
    return circuits;
  }

  /// @brief Extract a sparse shot from the simulator measurement record.
  /// @param circuit Circuit whose detectors/observables to evaluate.
  /// @return Sparse detector hits and observable mask.
  HOST auto getShot(const stim::Circuit &circuit) -> stim::SparseShot {
    stim::SparseShot shot;
    shot.obs_mask = stim::simd_bits<64>(circuit.count_observables());

    uint32_t midx = 0;
    uint32_t didx = 0;
    circuit.for_each_operation([&](const stim::CircuitInstruction op) {
      switch (op.gate_type) {
      case stim::GateType::M:
        midx += (uint32_t)op.targets.size();
        break;
      case stim::GateType::DETECTOR: {
        bool val = false;
        for (const auto &target : op.targets) {
          const uint32_t lookback = target.data & ~stim::TARGET_RECORD_BIT;
          const uint32_t m = midx - lookback;
          val ^= sim.measurement_record.storage[m];
        }
        if (val) {
          shot.hits.push_back(didx);
        }
        ++didx;
        break;
      }
      case stim::GateType::OBSERVABLE_INCLUDE: {
        bool val = false;
        for (const auto &target : op.targets) {
          const uint32_t lookback = target.data & ~stim::TARGET_RECORD_BIT;
          const uint32_t m = midx - lookback;
          val ^= sim.measurement_record.storage[m];
        }
        const auto obs = (uint32_t)op.args[0];
        if (val) {
          shot.obs_mask[obs] ^= true;
        }
        break;
      }
      default:
        break;
      }
    });
    return shot;
  }

  /// @brief Reset the simulator and pending inner corrections.
  HOST void reset() {
    const uint32_t numAll = (uint32_t)qubitIDs.all.size();
    const uint32_t numInnerXChecks = (uint32_t)qubitIDs.innerXChecks.size();
    const uint32_t numInnerZChecks = (uint32_t)qubitIDs.innerZChecks.size();

    sim.measurement_record.clear();
    sim.inv_state = stim::Tableau<64>::identity(numAll);
    sim.last_correlated_error_occurred = false;
    innerXCorrections.assign(numInnerXChecks, 0);
    innerZCorrections.assign(numInnerZChecks, 0);
  }

  /// @brief Get one memory experiment shot.
  /// @param rounds Number of QEC rounds.
  /// @param p Physical error rate.
  /// @param strategy Outer-check measurement strategy.
  /// @param includeXDetectors Include X-check detectors each round.
  /// @return Circuits and sparse shot for one memory experiment.
  HOST auto
  getMemory(uint32_t rounds,
            double p,
            MeasurementStrategy strategy = MeasurementStrategy::Static,
            bool includeXDetectors = true)
      -> std::pair<CircuitPair, stim::SparseShot> {
    reset();
    const auto head = getHead(p);
    const auto cycle = getCycle(rounds, p, strategy, includeXDetectors);
    const auto tail = getTail(rounds, p);
    applyInnerCorrections();
    const auto circuits = head + cycle + tail;
    const auto shot = getShot(circuits.circl);
    return {circuits, shot};
  }
};

} // namespace gp

#endif // GREENPEAS_QEC_CODES_CONCATENATEDSURFACECODE_HPP

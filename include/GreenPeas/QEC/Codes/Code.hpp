#ifndef GREENPEAS_QEC_CODES_CODE_HPP
#define GREENPEAS_QEC_CODES_CODE_HPP

/// Standard headers
#include <algorithm>
#include <cstdint>
#include <map>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

/// Project headers
#include "GreenPeas/Core/SparseMatrix.hpp"
#include "GreenPeas/Policies/Data/Format.hpp"
#include "GreenPeas/Policies/Storage/Host.hpp"

/// Third-party headers
#include "stim.h"

namespace gp {

/// @brief Parity-check matrix.
using PCM = SparseMatrix<uint32_t, uint32_t, HostStorage, COOFormat>;

/// @brief Logical-check matrix.
using LCM = SparseMatrix<uint32_t, uint8_t, HostStorage, COOFormat>;

/// @brief Collection of qubits indices
using Qubits = std::vector<uint32_t>;

/// @brief Layered CNOT schedule.
using CNOTSchedule = std::vector<Qubits>;

/// @brief Data qubits supported by each X/Z check.
using CheckSupport = std::vector<Qubits>;

/// @brief Data qubits supported by each X/Z logical.
using LogicalSupport = std::map<uint32_t, Qubits>;

/// @brief Supported code types.
enum class CodeType : uint32_t { Surface = 0, BB = 1 };

/// @brief Pauli basis.
enum class PauliBasis { X, Z };

/// @brief Convert a `CodeType` to its on-disk directory name.
/// @param type Code type.
/// @return Directory name string for `type`.
HOST inline auto toString(CodeType type) -> const char * {
  switch (type) {
  case CodeType::Surface:
    return "surface";
  case CodeType::BB:
    return "bb";
  default:
    throw std::invalid_argument("Unknown code type.");
  }
}

/// @brief Get the directory for code `type` at distance `d` under `root`.
/// @param type Code type.
/// @param d Code distance.
/// @param root Root data directory.
/// @return Absolute path to the code data directory.
HOST inline auto getCodePath(CodeType type, uint32_t d, const std::string &root)
    -> std::string {
  return root + "/codes/" + toString(type) + "/d" + std::to_string(d);
}

/// @brief Qubit IDs for a stabiliser code layout.
struct QubitIDs {
  /// @brief All qubit indices.
  Qubits all;

  /// @brief Data qubit indices.
  Qubits data;

  /// @brief All check ancilla indices.
  Qubits checks;

  /// @brief X-check ancilla indices.
  Qubits xChecks;

  /// @brief Z-check ancilla indices.
  Qubits zChecks;
};

/// @brief Get qubit IDs for a stabiliser code layout.
/// @param numData Number of data qubits.
/// @param numXChecks Number of X-check ancillas.
/// @param numZChecks Number of Z-check ancillas.
/// @return Contiguous qubit ID layout.
HOST inline auto getQubitIds(uint32_t numData,
                             uint32_t numXChecks,
                             uint32_t numZChecks) -> QubitIDs {
  const uint32_t numChecks = numXChecks + numZChecks;
  const uint32_t numAll = numData + numChecks;

  QubitIDs ids;
  ids.all.resize(numAll);
  std::iota(ids.all.begin(), ids.all.end(), 0);

  ids.data.resize(numData);
  std::iota(ids.data.begin(), ids.data.end(), 0);

  ids.checks.resize(numChecks);
  std::iota(ids.checks.begin(), ids.checks.end(), numData);

  ids.xChecks.resize(numXChecks);
  std::iota(ids.xChecks.begin(), ids.xChecks.end(), numData);

  ids.zChecks.resize(numZChecks);
  std::iota(ids.zChecks.begin(), ids.zChecks.end(), numData + numXChecks);

  return ids;
}

/// @brief Get the check support and CNOT schedule from a parity-check matrix.
/// @param pcm Parity-check matrix.
/// @param basis Pauli basis (X/Z).
/// @param checkIds Qubit ID for each check row in parity-check matrix.
/// @param support Data qubits supported by each X/Z check.
/// @param schedule Layered CNOT schedule.
HOST inline void getCheckSupportAndSchedule(const PCM &pcm,
                                            PauliBasis basis,
                                            const Qubits &checkIds,
                                            CheckSupport &support,
                                            CNOTSchedule &schedule) {
  const uint32_t numChecks = pcm.dimensions.numRows;
  support.assign(numChecks, {});

  uint32_t numLayers = 0;
  for (uint32_t i = 0; i < pcm.dimensions.numNonZeros; ++i) {
    numLayers = std::max(numLayers, pcm.vals[i] + 1);
  }
  schedule.assign(numLayers, {});

  for (uint32_t i = 0; i < pcm.dimensions.numNonZeros; ++i) {
    const uint32_t check = pcm.rows[i];
    const uint32_t data = pcm.cols[i];
    const uint32_t layer = pcm.vals[i];
    const uint32_t ancilla = checkIds[check];

    support[check].push_back(data);

    if (basis == PauliBasis::X) {
      schedule[layer].push_back(ancilla);
      schedule[layer].push_back(data);
    } else {
      schedule[layer].push_back(data);
      schedule[layer].push_back(ancilla);
    }
  }
}

/// @brief Get the logical support from a logical-check matrix.
/// @param lcm Logical-check matrix.
/// @param support Output logical support.
HOST inline void getLogicalSupport(const LCM &lcm, LogicalSupport &support) {
  support.clear();
  for (uint32_t i = 0; i < lcm.dimensions.numNonZeros; ++i) {
    support[lcm.rows[i]].push_back(lcm.cols[i]);
  }
}

/// @brief Get the qubit indices in `all` that are not in `active`.
/// @param all Full list of qubit indices (must be sorted).
/// @param active Indices of the active qubits.
/// @param idle Indices of the idle qubits (output).
HOST inline void
getIdleQubits(const Qubits &all, const Qubits &active, Qubits &idle) {
  Qubits sortedActive(active);
  std::sort(sortedActive.begin(), sortedActive.end());
  idle.clear();
  std::set_difference(all.begin(),
                      all.end(),
                      sortedActive.begin(),
                      sortedActive.end(),
                      std::back_inserter(idle));
}

/// @brief Append noisy CNOT layers.
/// @param circuit Stim circuit to extend.
/// @param schedule Layered CNOT schedule.
/// @param p Physical error rate.
/// @param all Full list of qubit indices (must be sorted).
/// @param idle Reusable buffer for idle qubits each layer.
HOST inline void appendCNOTLayers(stim::Circuit &circuit,
                                  const CNOTSchedule &schedule,
                                  double p,
                                  const Qubits &all,
                                  Qubits &idle) {
  for (const auto &active : schedule) {
    circuit.safe_append_u("CNOT", active);
    circuit.safe_append_u("TICK", {});
    circuit.safe_append_u("DEPOLARIZE2", active, {p});
    getIdleQubits(all, active, idle);
    if (!idle.empty()) {
      circuit.safe_append_u("DEPOLARIZE1", idle, {p / 10});
    }
  }
}

/// @brief Append detectors for a range  [`begin, `end`) of stabiliser checks.
/// @param circuit Stim circuit to extend.
/// @param numChecks Total number of X and Z checks.
/// @param begin First check index.
/// @param end One past the last check index.
HOST inline void appendDetectors(stim::Circuit &circuit,
                                 uint32_t numChecks,
                                 uint32_t begin,
                                 uint32_t end) {
  for (uint32_t i = begin; i < end; ++i) {
    circuit.safe_append_u(
        "DETECTOR",
        {(uint32_t)(numChecks * 1 - i) | stim::TARGET_RECORD_BIT,
         (uint32_t)(numChecks * 2 - i) | stim::TARGET_RECORD_BIT});
  }
}

/// @brief Stabilizer QEC code loaded from precomputed `.mtx` data.
struct Code {
  /// @brief X-parity-check matrix.
  PCM Hx;

  /// @brief Z-parity-check matrix.
  PCM Hz;

  /// @brief X-logical-check matrix.
  LCM Lx;

  /// @brief Z-logical-check matrix.
  LCM Lz;

  /// @brief Qubit index layout.
  QubitIDs qubitIDs;

  /// @brief Layered CNOT schedule for X checks.
  CNOTSchedule checkXSchedule;

  /// @brief Layered CNOT schedule for Z checks.
  CNOTSchedule checkZSchedule;

  /// @brief Data qubits per X check.
  CheckSupport checkXSupport;

  /// @brief Data qubits per Z check.
  CheckSupport checkZSupport;

  /// @brief Data qubits per X logical.
  LogicalSupport logicalXSupport;

  /// @brief Data qubits per Z logical.
  LogicalSupport logicalZSupport;

  /// @brief Construct a stabiliser code of `type` with distance `d`.
  /// @param type Code type.
  /// @param d Code distance.
  /// @param root Root data directory.
  HOST explicit Code(CodeType type, uint32_t d, const std::string &root)
      : Hx(PCM::fromMtx(getCodePath(type, d, root) + "/Hx.mtx")),
        Hz(PCM::fromMtx(getCodePath(type, d, root) + "/Hz.mtx")),
        Lx(LCM::fromMtx(getCodePath(type, d, root) + "/Lx.mtx")),
        Lz(LCM::fromMtx(getCodePath(type, d, root) + "/Lz.mtx")) {
    init();
  }

  /// @brief Initialise qubit IDs, schedules, and supports from loaded matrices.
  HOST void init() {
    qubitIDs = getQubitIds(
        Hx.dimensions.numCols, Hx.dimensions.numRows, Hz.dimensions.numRows);

    getCheckSupportAndSchedule(
        Hx, PauliBasis::X, qubitIDs.xChecks, checkXSupport, checkXSchedule);
    getCheckSupportAndSchedule(
        Hz, PauliBasis::Z, qubitIDs.zChecks, checkZSupport, checkZSchedule);

    getLogicalSupport(Lx, logicalXSupport);
    getLogicalSupport(Lz, logicalZSupport);
  }

  /// @brief Get the head of the circuit.
  /// @param p Physical error rate.
  /// @return Stim circuit fragment for state preparation.
  HOST auto getHead(double p) const -> stim::Circuit {
    stim::Circuit circuit;
    circuit.safe_append_u("R", qubitIDs.data);
    circuit.safe_append_u("TICK", {});
    circuit.safe_append_u("X_ERROR", qubitIDs.data, {p});
    circuit.safe_append_u("DEPOLARIZE1", qubitIDs.checks, {p / 10});
    return circuit;
  }

  /// @brief Get one QEC round.
  /// @param p Physical error rate.
  /// @return Stim circuit fragment for one stabilizer round.
  HOST auto getQECRound(double p) const -> stim::Circuit {
    stim::Circuit circuit;

    Qubits idle;
    idle.reserve(qubitIDs.all.size());

    circuit.safe_append_u("R", qubitIDs.checks);
    circuit.safe_append_u("TICK", {});
    circuit.safe_append_u("X_ERROR", qubitIDs.checks, {p});
    circuit.safe_append_u("DEPOLARIZE1", qubitIDs.data, {p / 10});

    if (!qubitIDs.xChecks.empty()) {
      circuit.safe_append_u("H", qubitIDs.xChecks);
      circuit.safe_append_u("TICK", {});
      circuit.safe_append_u("DEPOLARIZE1", qubitIDs.all, {p / 10});
    }

    appendCNOTLayers(circuit, checkXSchedule, p, qubitIDs.all, idle);
    appendCNOTLayers(circuit, checkZSchedule, p, qubitIDs.all, idle);

    if (!qubitIDs.xChecks.empty()) {
      circuit.safe_append_u("H", qubitIDs.xChecks);
      circuit.safe_append_u("TICK", {});
      circuit.safe_append_u("DEPOLARIZE1", qubitIDs.all, {p / 10});
    }

    circuit.safe_append_u("M", qubitIDs.checks, {p});
    return circuit;
  }

  /// @brief Get `rounds` QEC rounds with detector annotations between rounds.
  /// @param rounds Number of QEC rounds.
  /// @param p Physical error rate.
  /// @param includeXDetectors Include X-check detectors each round.
  /// @return Stim circuit fragment for `rounds` annotated QEC rounds.
  HOST auto getQECRounds(uint32_t rounds,
                         double p,
                         bool includeXDetectors) const -> stim::Circuit {
    const auto oneQECRound = getQECRound(p);

    const uint32_t numChecks = (uint32_t)qubitIDs.checks.size();
    const uint32_t numXChecks = (uint32_t)qubitIDs.xChecks.size();

    stim::Circuit circuit = oneQECRound;

    for (size_t i = 0; i < qubitIDs.zChecks.size(); ++i) {
      circuit.safe_append_u(
          "DETECTOR",
          {(uint32_t)(qubitIDs.zChecks.size() - i) | stim::TARGET_RECORD_BIT});
    }

    for (uint32_t r = 1; r < rounds; ++r) {
      circuit += oneQECRound;

      if (includeXDetectors) {
        appendDetectors(circuit, numChecks, 0, numXChecks);
      }
      appendDetectors(circuit, numChecks, numXChecks, numChecks);
    }

    return circuit;
  }

  /// @brief Get the tail of the circuit.
  /// @param p Physical error rate.
  /// @return Stim circuit fragment for final data measurement.
  HOST auto getTail(double p) const -> stim::Circuit {
    stim::Circuit circuit;
    const uint32_t numData = (uint32_t)qubitIDs.data.size();
    const uint32_t numZChecks = (uint32_t)qubitIDs.zChecks.size();

    circuit.safe_append_u("M", qubitIDs.data, {p});

    for (uint32_t check = 0; check < numZChecks; ++check) {
      const auto &support = checkZSupport[check];
      if (support.empty()) {
        continue;
      }
      Qubits records;
      records.reserve(support.size() + 1);
      for (const uint32_t q : support) {
        records.push_back((numData - q) | stim::TARGET_RECORD_BIT);
      }
      records.push_back((numData + numZChecks - check) |
                        stim::TARGET_RECORD_BIT);
      circuit.safe_append_u("DETECTOR", records);
    }

    for (const auto &[logical, support] : logicalZSupport) {
      Qubits records;
      records.reserve(support.size());
      for (const uint32_t q : support) {
        records.push_back((numData - q) | stim::TARGET_RECORD_BIT);
      }
      circuit.safe_append_u("OBSERVABLE_INCLUDE", records, {(double)logical});
    }

    return circuit;
  }

  /// @brief Get the full memory experiment circuit.
  /// @param rounds Number of QEC rounds.
  /// @param p Physical error rate.
  /// @param includeXDetectors Include X-check detectors each round.
  /// @return Full Stim memory-experiment circuit.
  HOST auto getMemory(uint32_t rounds,
                      double p,
                      bool includeXDetectors = true) const -> stim::Circuit {
    return getHead(p) + getQECRounds(rounds, p, includeXDetectors) + getTail(p);
  }
};

/// @brief Rotated planar surface code.
struct SurfaceCode : Code {
  /// @brief Construct a surface code with distance @p d under @p root.
  /// @param d Code distance.
  /// @param root Root data directory.
  HOST explicit SurfaceCode(uint32_t d, const std::string &root)
      : Code(CodeType::Surface, d, root) {}
};

/// @brief Bivariate bicycle (BB) code.
struct BBCode : Code {
  /// @brief Construct a BB code with distance @p d under @p root.
  /// @param d Code distance.
  /// @param root Root data directory.
  HOST explicit BBCode(uint32_t d, const std::string &root)
      : Code(CodeType::BB, d, root) {}
};

} // namespace gp

#endif // GREENPEAS_QEC_CODES_CODE_HPP

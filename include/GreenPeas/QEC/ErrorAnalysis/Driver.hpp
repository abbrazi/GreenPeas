#ifndef GREENPEAS_QEC_ERRORANALYSIS_DRIVER_HPP
#define GREENPEAS_QEC_ERRORANALYSIS_DRIVER_HPP

/// Project headers
#include "GreenPeas/Core/Graph.hpp"
#include "GreenPeas/QEC/ErrorAnalysis/Circuit.hpp"
#include "GreenPeas/QEC/ErrorAnalysis/Workspace.hpp"

namespace gp {

/// @brief Non-owning view passed to compute backends.
template <typename Layout>
struct DriverView {
  /// @brief
  GraphView graph;

  /// @brief
  SensitivityWorkspaceView<Layout> sense;

  /// @brief
  ErrorWorkspaceView<Layout> error;

  /// @brief
  ScratchpadView scratchpad;
};

/// @brief
/// @tparam Storage
/// @tparam Compute
/// @tparam Layout
template <typename Storage,
          typename Compute,
          typename Layout,
          CorrelationLevel Level,
          size_t W = 32>
struct Driver {
  /// @brief
  Graph<Storage> graph;

  /// @brief
  SensitivityWorkspace<Storage, Layout> sense;

  /// @brief
  ErrorWorkspace<Storage, Layout, W> error;

  /// @brief
  HypergraphWorkspace<Layout, W> model;

  /// @brief
  size_t sortBytes{};

  /// @brief
  size_t reduceBytes{};

  /// @brief
  Scratchpad<Storage> scratchpad;

  /// @brief
  HOST explicit Driver(CircuitParameters<Level> parameters)
      : graph(parameters.numNodes()), sense(parameters.numNodes(),
                                            parameters.numMeasurements,
                                            parameters.numWordsPerNode()),
        error(parameters.numNodes()), model(parameters.numNodes()),
        scratchpad(getScratchpadSize()) {}

  /// @brief
  /// @return
  HOST auto getView() -> DriverView<Layout> {
    return {graph.getView(),
            sense.getView(),
            error.getView(),
            scratchpad.getView()};
  }

  /// @brief
  /// @return
  HOST auto getScratchpadSize() -> uint32_t {
    auto view = getView();

    return Compute::getScratchpadSize(view, error.hashes.a.size, sortBytes,
                                      reduceBytes);
  }

  /// @brief
  HOST void reset() {
    sense.clear();
    error.clear();
  }

  /// @brief Fit buffers to circuit parameters.
  /// @param parameters Circuit parameters.
  HOST void fitto(CircuitParameters<Level> parameters) {
    graph.fitto(parameters.numNodes());
    sense.fitto(parameters.numNodes(),
                parameters.numMeasurements,
                parameters.numWordsPerNode());
    error.fitto(parameters.numNodes());
    model.fitto(parameters.numNodes());
    scratchpad.fitto(getScratchpadSize());
  }

  /// @brief
  /// @return
  HOST void runComputePipeline(CircuitParameters<Level> parameters) {
    auto world = getView();

    const uint32_t nLayers = parameters.numLayers;
    const uint32_t nMeasurements = parameters.numMeasurements;
    const uint32_t nNodesPerLayer = parameters.numNodesPerLayer();
    const uint32_t nNodes = parameters.numNodes();
    const uint32_t nWordsPerNode = parameters.numWordsPerNode();

    Compute::scatterInitialSensitivities(world, nMeasurements, nWordsPerNode);
    Compute::genSensitivities(world, nLayers, nNodesPerLayer, nWordsPerNode);
    Compute::genErrorClasses(world, nNodes, nWordsPerNode);
    Compute::sortErrorClasses(world, nNodes, sortBytes);
    Compute::permuteErrorProbabilities(world, nNodes);
    Compute::reduceErrorClasses(world, nNodes, reduceBytes);

    error.numErrorClasses.copyTo(model.numClasses);

    Compute::gatherFinalErrorClasses(world, model.numClasses, W);
  }

  /// @brief
  /// @return
  HOST void compile(const Circuit<Layout, Level> &circuit) {
    // 0. Reset persistent state
    reset();

    // 1. Resize all buffers to match circuit parameters
    fitto(circuit.parameters);

    // 2. Copy graph
    graph.copyFrom(circuit.stepg.graph);

    // 3. Copy initial probabilities
    error.probabilities.a.copyFrom(circuit.stepg.probs);

    // 4. Copy sensitivity map
    sense.map.copyFrom(circuit.sensitivityMap);

    // 5. Delegate core workload to `Compute`
    runComputePipeline(circuit.parameters);

    // 6. Copy final error classes
    error.classes.b.copyTo(model.classes);

    // 7. Copy final error probabilities
    error.probabilities.a.copyTo(model.probabilities);
  }

  /// @brief
  /// @return
  HOST auto getDEM(uint32_t numDetectors) -> stim::DetectorErrorModel {
    auto getTarget = [&](uint32_t d) {
      return d >= numDetectors
                 ? stim::DemTarget::observable_id(d - numDetectors)
                 : stim::DemTarget::relative_detector_id(d);
    };

    stim::DetectorErrorModel dem;
    for (uint32_t row = 0; row < model.numClasses; ++row) {
      double p = model.probabilities[row];
      std::vector<stim::DemTarget> targets;
      targets.reserve(W);
      for (uint32_t col = 0; col < W; ++col) {
        uint32_t d = model.classes(row, col);
        if (d != UINT32_MAX) {
          targets.emplace_back(getTarget(d));
        }
      }
      if (!targets.empty()) {
        dem.append_error_instruction(p, targets, "");
      }
    }

    return dem;
  }

  /// @brief
  /// @return
  HOST auto compile(const stim::Circuit &circuit) -> stim::DetectorErrorModel {
    auto native = Circuit<Layout, Level>::fromStimCircuit(circuit);
    compile(native);
    return getDEM(native.parameters.numDetectors);
  }

  /// @brief
  HOST static auto fromStimCircuit(const stim::Circuit &circuit) -> Driver<Storage, Compute, Layout, Level, W> {
    CircuitParameters<Level> parameters;

    const auto stats = circuit.compute_stats();

    parameters.numQubits = stats.num_qubits;
    parameters.numLayers = stats.num_ticks + 1;
    parameters.numMeasurements = stats.num_measurements;
    parameters.numDetectors = stats.num_detectors;
    parameters.numObservables = stats.num_observables;

    return Driver<Storage, Compute, Layout, Level, W>(parameters);
  }
};

} // namespace gp

#endif // GREENPEAS_QEC_ERRORANALYSIS_DRIVER_HPP

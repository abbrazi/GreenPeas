#ifndef GREENPEAS_QEC_ERRORANALYSIS_DRIVER_HPP
#define GREENPEAS_QEC_ERRORANALYSIS_DRIVER_HPP

/// Standard headers
#include <utility>

/// Project headers
#include "GreenPeas/Core/Graph.hpp"
#include "GreenPeas/QEC/ErrorAnalysis/Circuit.hpp"
#include "GreenPeas/QEC/ErrorAnalysis/Workspace.hpp"

namespace gp {

/// @brief Non-owning view of driver graph, sense, error, and scratch buffers.
/// @tparam Layout Layout policy.
template <typename Layout>
struct DriverView {
  /// @brief STEPG adjacency view used by compute kernels.
  GraphView graph;

  /// @brief Sensitivity workspace view.
  SensitivityWorkspaceView<Layout> sense;

  /// @brief Error-class workspace view.
  ErrorWorkspaceView<Layout> error;

  /// @brief Compute scratchpad view.
  ScratchpadView scratchpad;
};

/// @brief Error-analysis driver that compiles a Stim circuit to a DEM.
/// @tparam Storage Storage policy for device/host buffers.
/// @tparam Compute Compute policy (e.g. `CUDACompute`, `HostCompute`).
/// @tparam Layout Layout policy.
/// @tparam Level Correlation level.
/// @tparam W Maximum packed class width.
template <typename Storage,
          typename Compute,
          typename Layout,
          CorrelationLevel Level,
          size_t W = 32>
struct Driver {
  /// @brief Parsed syndrome-measurement circuit.
  Circuit<Layout, Level> circuit;

  /// @brief Working copy of the STEPG adjacency.
  Graph<Storage> graph;

  /// @brief Sensitivity map and dense matrix workspace.
  SensitivityWorkspace<Storage, Layout> sense;

  /// @brief Error-class hashing and reduction workspace.
  ErrorWorkspace<Storage, Layout, W> error;

  /// @brief Host-side reduced detector-error hypergraph.
  HypergraphWorkspace<Layout, W> model;

  /// @brief CUB temporary storage size for radix sort.
  size_t sortBytes{};

  /// @brief CUB temporary storage size for reduce-by-key.
  size_t reduceBytes{};

  /// @brief Scratch buffer sized for sort/reduce.
  Scratchpad<Storage> scratchpad;

  /// @brief Construct a driver around an existing SM circuit.
  /// @param circuit Parsed syndrome-measurement circuit (moved).
  HOST explicit Driver(Circuit<Layout, Level> circuit)
      : circuit(std::move(circuit)), graph(this->circuit.parameters.numNodes()),
        sense(this->circuit.parameters.numNodes(),
              this->circuit.parameters.numMeasurements,
              this->circuit.parameters.numWordsPerNode()),
        error(this->circuit.parameters.numNodes()),
        model(this->circuit.parameters.numNodes()),
        scratchpad(getScratchpadSize()) {}

  /// @brief Get a non-owning view of this driver's buffers.
  /// @return DriverView sharing this driver's storage.
  HOST auto getView() -> DriverView<Layout> {
    return {graph.getView(),
            sense.getView(),
            error.getView(),
            scratchpad.getView()};
  }

  /// @brief Query and cache CUB scratch sizes; return max bytes needed.
  /// @return `max(sortBytes, reduceBytes)`.
  HOST auto getScratchpadSize() -> uint32_t {
    auto view = getView();

    return Compute::getScratchpadSize(
        view, error.hashes.a.size, sortBytes, reduceBytes);
  }

  /// @brief Clear sensitivity and error workspaces for reuse.
  HOST void reset() {
    sense.clear();
    error.clear();
  }

  /// @brief Resize all buffers to match @p parameters.
  /// @param parameters Circuit parameters driving buffer sizes.
  HOST void fitto(CircuitParameters<Level> parameters) {
    graph.fitto(parameters.numNodes());
    sense.fitto(parameters.numNodes(),
                parameters.numMeasurements,
                parameters.numWordsPerNode());
    error.fitto(parameters.numNodes());
    model.fitto(parameters.numNodes());
    scratchpad.fitto(getScratchpadSize());
  }

  /// @brief Run the compute pipeline that builds reduced error classes.
  /// @param parameters Circuit parameters for the current compile.
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

  /// @brief Compile `circuit` into the reduced detector-error hypergraph.
  HOST void compile() {
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

  /// @brief Build a Stim detector error model from the compiled hypergraph.
  /// @return Stim `DetectorErrorModel` with one error per reduced class.
  HOST auto getDetectorErrorModel() -> stim::DetectorErrorModel {
    const uint32_t numDetectors = circuit.parameters.numDetectors;

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

  /// @brief Parse @p stimCircuit, compile, and return its DEM.
  /// @param stimCircuit Input Stim circuit.
  /// @return Compiled Stim `DetectorErrorModel`.
  HOST auto compileDetectorErrorModel(const stim::Circuit &stimCircuit)
      -> stim::DetectorErrorModel {
    circuit.parseFromStimCircuit(stimCircuit);
    compile();
    return getDetectorErrorModel();
  }

  /// @brief Compile the currently parsed circuit and return its DEM.
  /// @return Compiled Stim `DetectorErrorModel`.
  HOST auto compileDetectorErrorModel() -> stim::DetectorErrorModel {
    compile();
    return getDetectorErrorModel();
  }

  /// @brief Construct a driver by parsing @p stimCircuit.
  /// @param stimCircuit Input Stim circuit.
  /// @return Driver owning the parsed SM circuit and workspaces.
  HOST static auto fromStimCircuit(const stim::Circuit &stimCircuit) -> Driver {
    return Driver(Circuit<Layout, Level>::fromStimCircuit(stimCircuit));
  }
};

} // namespace gp

#endif // GREENPEAS_QEC_ERRORANALYSIS_DRIVER_HPP

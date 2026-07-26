#ifndef GREENPEAS_QEC_ERRORANALYSIS_DRIVER_HPP
#define GREENPEAS_QEC_ERRORANALYSIS_DRIVER_HPP

/// Standard headers
#include <utility>

/// Project headers
#include "GreenPeas/Core/Graph.hpp"
#include "GreenPeas/QEC/ErrorAnalysis/Circuit.hpp"
#include "GreenPeas/QEC/ErrorAnalysis/Workspace.hpp"

namespace gp {

template <typename Layout>
struct DriverView {
  GraphView graph;

  SensitivityWorkspaceView<Layout> sense;

  ErrorWorkspaceView<Layout> error;

  ScratchpadView scratchpad;
};

template <typename Storage,
          typename Compute,
          typename Layout,
          CorrelationLevel Level,
          size_t W = 32>
struct Driver {
  Circuit<Layout, Level> circuit;

  Graph<Storage> graph;

  SensitivityWorkspace<Storage, Layout> sense;

  ErrorWorkspace<Storage, Layout, W> error;

  HypergraphWorkspace<Layout, W> model;

  size_t sortBytes{};

  size_t reduceBytes{};

  Scratchpad<Storage> scratchpad;

  HOST explicit Driver(Circuit<Layout, Level> circuit)
      : circuit(std::move(circuit)), graph(this->circuit.parameters.numNodes()),
        sense(this->circuit.parameters.numNodes(),
              this->circuit.parameters.numMeasurements,
              this->circuit.parameters.numWordsPerNode()),
        error(this->circuit.parameters.numNodes()),
        model(this->circuit.parameters.numNodes()),
        scratchpad(getScratchpadSize()) {}

  HOST auto getView() -> DriverView<Layout> {
    return {graph.getView(),
            sense.getView(),
            error.getView(),
            scratchpad.getView()};
  }

  HOST auto getScratchpadSize() -> uint32_t {
    auto view = getView();

    return Compute::getScratchpadSize(
        view, error.hashes.a.size, sortBytes, reduceBytes);
  }

  HOST void reset() {
    sense.clear();
    error.clear();
  }

  HOST void fitto(CircuitParameters<Level> parameters) {
    graph.fitto(parameters.numNodes());
    sense.fitto(parameters.numNodes(),
                parameters.numMeasurements,
                parameters.numWordsPerNode());
    error.fitto(parameters.numNodes());
    model.fitto(parameters.numNodes());
    scratchpad.fitto(getScratchpadSize());
  }

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

  HOST auto compileDetectorErrorModel(const stim::Circuit &stimCircuit)
      -> stim::DetectorErrorModel {
    circuit.parseFromStimCircuit(stimCircuit);
    compile();
    return getDetectorErrorModel();
  }

  HOST static auto fromStimCircuit(const stim::Circuit &stimCircuit) -> Driver {
    return Driver(Circuit<Layout, Level>::fromStimCircuit(stimCircuit));
  }
};

} // namespace gp

#endif // GREENPEAS_QEC_ERRORANALYSIS_DRIVER_HPP

#include "GreenPeas/QEC/ErrorAnalysis/ErrorAnalysis.pybind.hpp"

#include <memory>
#include <stdexcept>
#include <variant>

#include "GreenPeas/Policies/Data/Layout.hpp"
#include "GreenPeas/QEC/Shims/Stim.pybind.hpp"
#include "GreenPeas/QEC/ErrorAnalysis/Driver.hpp"
#include "GreenPeas/QEC/ErrorAnalysis/Mixer.hpp"

#include "GreenPeas/Policies/Compute/CUDA.hpp"
#include "GreenPeas/Policies/Storage/CUDA.hpp"

namespace py = pybind11;

namespace gp {

template <CorrelationLevel Level>
using CUDADriver = Driver<CUDAStorage, CUDACompute, ColMajorLayout, Level>;

using CUDADriverVariant = std::variant<CUDADriver<CorrelationLevel::L0>,
                                       CUDADriver<CorrelationLevel::L1>,
                                       CUDADriver<CorrelationLevel::L2>>;

auto getDriver(const stim::Circuit &circuit, CorrelationLevel level)
    -> std::unique_ptr<CUDADriverVariant> {
  switch (level) {
  case CorrelationLevel::L0:
    return std::make_unique<CUDADriverVariant>(
        CUDADriver<CorrelationLevel::L0>::fromStimCircuit(circuit));
  case CorrelationLevel::L1:
    return std::make_unique<CUDADriverVariant>(
        CUDADriver<CorrelationLevel::L1>::fromStimCircuit(circuit));
  case CorrelationLevel::L2:
    return std::make_unique<CUDADriverVariant>(
        CUDADriver<CorrelationLevel::L2>::fromStimCircuit(circuit));
  }

  throw std::invalid_argument("Unsupported correlation level.");
}

} // namespace gp

namespace gp::pybind {

void bindErrorAnalysis(py::module_ &error_analysis) {
  py::enum_<CorrelationLevel>(error_analysis, "CorrelationLevel")
      .value("L0", CorrelationLevel::L0)
      .value("L1", CorrelationLevel::L1)
      .value("L2", CorrelationLevel::L2)
      .export_values();

  py::class_<CUDADriverVariant, std::unique_ptr<CUDADriverVariant>>(
      error_analysis, "Driver")
      .def(
          "compile_detector_error_model",
          [](CUDADriverVariant &driver, const py::object &circuit) {
            return demToPython(std::visit(
                [&](auto &d) {
                  return d.compileDetectorErrorModel(
                      circuitFromPython(circuit));
                },
                driver));
          },
          py::arg("circuit"));

  error_analysis.def(
      "get_driver",
      [](const py::object &circuit, CorrelationLevel correlation_level) {
        return getDriver(circuitFromPython(circuit), correlation_level);
      },
      py::arg("circuit"),
      py::arg("correlation_level") = CorrelationLevel::L2);
}

} // namespace gp::pybind

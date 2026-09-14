#include "GreenPeas/QEC/ErrorAnalysis/ErrorAnalysis.pybind.hpp"

#include <memory>
#include <stdexcept>
#include <variant>

#include "GreenPeas/Policies/Data/Layout.hpp"
#include "GreenPeas/QEC/ErrorAnalysis/Circuit.hpp"
#include "GreenPeas/QEC/ErrorAnalysis/Driver.hpp"
#include "GreenPeas/QEC/ErrorAnalysis/Mixer.hpp"
#include "GreenPeas/QEC/Shims/Stim.pybind.hpp"

#include "GreenPeas/Policies/Compute/Host.hpp"
#include "GreenPeas/Policies/Storage/Host.hpp"

namespace py = pybind11;

namespace gp {

template <CorrelationLevel Level>
using HostCircuit = Circuit<RowMajorLayout, Level>;

using HostCircuitVariant = std::variant<HostCircuit<CorrelationLevel::L0>,
                                        HostCircuit<CorrelationLevel::L1>,
                                        HostCircuit<CorrelationLevel::L2>>;

template <CorrelationLevel Level>
using HostDriver = Driver<HostStorage, HostCompute, RowMajorLayout, Level>;

using HostDriverVariant = std::variant<HostDriver<CorrelationLevel::L0>,
                                       HostDriver<CorrelationLevel::L1>,
                                       HostDriver<CorrelationLevel::L2>>;

auto getNativeCircuit(const stim::Circuit &circuit, CorrelationLevel level)
    -> std::unique_ptr<HostCircuitVariant> {
  switch (level) {
  case CorrelationLevel::L0:
    return std::make_unique<HostCircuitVariant>(
        HostCircuit<CorrelationLevel::L0>::fromStimCircuit(circuit));
  case CorrelationLevel::L1:
    return std::make_unique<HostCircuitVariant>(
        HostCircuit<CorrelationLevel::L1>::fromStimCircuit(circuit));
  case CorrelationLevel::L2:
    return std::make_unique<HostCircuitVariant>(
        HostCircuit<CorrelationLevel::L2>::fromStimCircuit(circuit));
  }

  throw std::invalid_argument("Unsupported correlation level.");
}

auto getDriver(const stim::Circuit &circuit, CorrelationLevel level)
    -> std::unique_ptr<HostDriverVariant> {
  switch (level) {
  case CorrelationLevel::L0:
    return std::make_unique<HostDriverVariant>(
        HostDriver<CorrelationLevel::L0>::fromStimCircuit(circuit));
  case CorrelationLevel::L1:
    return std::make_unique<HostDriverVariant>(
        HostDriver<CorrelationLevel::L1>::fromStimCircuit(circuit));
  case CorrelationLevel::L2:
    return std::make_unique<HostDriverVariant>(
        HostDriver<CorrelationLevel::L2>::fromStimCircuit(circuit));
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

  py::class_<HostCircuitVariant, std::unique_ptr<HostCircuitVariant>>(
      error_analysis, "Circuit")
      .def(py::init([](const py::object &circuit,
                       CorrelationLevel correlation_level) {
             return getNativeCircuit(circuitFromPython(circuit),
                                     correlation_level);
           }),
           py::arg("circuit"),
           py::arg("correlation_level") = CorrelationLevel::L2)
      .def("get_xml", [](const HostCircuitVariant &circuit) {
        return std::visit([](const auto &native) { return native.getXML(); },
                          circuit);
      });

  py::class_<HostDriverVariant, std::unique_ptr<HostDriverVariant>>(
      error_analysis, "Driver")
      .def(
          "compile_detector_error_model",
          [](HostDriverVariant &driver, const py::object &circuit) {
            return demToPython(std::visit(
                [&](auto &d) {
                  return d.compileDetectorErrorModel(
                      circuitFromPython(circuit));
                },
                driver));
          },
          py::arg("circuit"))
      .def("compile_detector_error_model", [](HostDriverVariant &driver) {
        return demToPython(std::visit(
            [](auto &d) { return d.compileDetectorErrorModel(); }, driver));
      });

  error_analysis.def(
      "get_driver",
      [](const py::object &circuit, CorrelationLevel correlation_level) {
        return getDriver(circuitFromPython(circuit), correlation_level);
      },
      py::arg("circuit"),
      py::arg("correlation_level") = CorrelationLevel::L2);
}

} // namespace gp::pybind

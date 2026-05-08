#include "GreenPeas/QEC/Codes/Codes.pybind.hpp"
#include "GreenPeas/QEC/Codes/Code.hpp"

namespace py = pybind11;

namespace {

auto toPython(const stim::Circuit &circuit) -> py::object {
  return py::module_::import("stim").attr("Circuit")(circuit.str());
}

} // namespace

namespace gp::pybind {

void bindCodes(py::module_ &codes) {
  auto code = py::class_<Code>(codes, "Code");

  code.def(
      "get_memory",
      [](const Code &code,
         uint32_t rounds,
         double p,
         bool include_x_detectors) {
        const auto circuit = code.getMemory(rounds, p, include_x_detectors);
        return toPython(circuit);
      },
      py::arg("rounds"),
      py::arg("p"),
      py::arg("include_x_detectors") = true);

  py::class_<SurfaceCode, Code>(codes, "_SurfaceCode")
      .def(py::init<uint32_t, const std::string &>(),
           py::arg("distance"),
           py::arg("root"));

  py::class_<BBCode, Code>(codes, "_BBCode")
      .def(py::init<uint32_t, const std::string &>(),
           py::arg("distance"),
           py::arg("root"));
}

} // namespace gp::pybind

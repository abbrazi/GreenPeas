#include "GreenPeas/QEC/Codes/Codes.pybind.hpp"
#include "GreenPeas/QEC/Codes/Code.hpp"
#include "GreenPeas/QEC/Shims/Stim.pybind.hpp"

namespace py = pybind11;

namespace gp::pybind {

void bindCodes(py::module_ &codes) {
  auto code = py::class_<Code>(codes, "Code");

  code.def(
      "get_memory",
      [](const Code &code,
         uint32_t rounds,
         double p,
         bool include_x_detectors) {
        return circuitToPython(code.getMemory(rounds, p, include_x_detectors));
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

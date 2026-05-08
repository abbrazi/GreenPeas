#include <pybind11/pybind11.h>

#include "GreenPeas/QEC/Codes/Codes.pybind.hpp"
#include "GreenPeas/QEC/ErrorAnalysis/ErrorAnalysis.pybind.hpp"

namespace py = pybind11;

PYBIND11_MODULE(_gppy, m) {
  m.doc() = "GreenPeas internal extension module";

  auto codes = m.def_submodule("codes", "Stabilizer QEC codes.");
  gp::pybind::bindCodes(codes);

  auto error_analysis = m.def_submodule("error_analysis", "Error analysis driver.");
  gp::pybind::bindErrorAnalysis(error_analysis);
}

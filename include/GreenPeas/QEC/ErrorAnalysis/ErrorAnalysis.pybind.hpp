#ifndef GREENPEAS_QEC_ERRORANALYSIS_PYBIND_HPP
#define GREENPEAS_QEC_ERRORANALYSIS_PYBIND_HPP

/// Third-party headers
#include <pybind11/pybind11.h>

namespace gp::pybind {

void bindErrorAnalysis(pybind11::module_ &error_analysis);

} // namespace gp::pybind

#endif // GREENPEAS_QEC_ERRORANALYSIS_PYBIND_HPP

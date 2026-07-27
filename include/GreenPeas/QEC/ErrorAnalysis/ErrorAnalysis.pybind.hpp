#ifndef GREENPEAS_QEC_ERRORANALYSIS_PYBIND_HPP
#define GREENPEAS_QEC_ERRORANALYSIS_PYBIND_HPP

/// Third-party headers
#include <pybind11/pybind11.h>

namespace gp::pybind {

/// @brief Bind error-analysis types into the `error_analysis` submodule.
/// @param error_analysis Target pybind11 module.
void bindErrorAnalysis(pybind11::module_ &error_analysis);

} // namespace gp::pybind

#endif // GREENPEAS_QEC_ERRORANALYSIS_PYBIND_HPP

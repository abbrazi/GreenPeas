#ifndef GREENPEAS_QEC_CODES_PYBIND_HPP
#define GREENPEAS_QEC_CODES_PYBIND_HPP

/// Third-party headers
#include <pybind11/pybind11.h>

namespace gp::pybind {

void bindCodes(pybind11::module_ &codes);

} // namespace gp::pybind

#endif // GREENPEAS_QEC_CODES_PYBIND_HPP

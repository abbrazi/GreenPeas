#ifndef GREENPEAS_QEC_SHIMS_STIM_PYBIND_HPP
#define GREENPEAS_QEC_SHIMS_STIM_PYBIND_HPP

#include <stdexcept>
#include <utility>

#include <pybind11/pybind11.h>

#include "stim.h"

namespace gp {

/// @brief Import stim so its pybind11 type casters are registered.
inline void ensureStimImported() { pybind11::module_::import("stim"); }

/// @brief Cast a Python stim.Circuit to a C++ stim::Circuit.
inline auto circuitFromPython(const pybind11::object &obj)
    -> const stim::Circuit & {
  ensureStimImported();
  try {
    return obj.cast<const stim::Circuit &>();
  } catch (const pybind11::cast_error &) {
    throw std::runtime_error(
        "Failed to cast to stim.Circuit from Python. "
        "Ensure stim is installed and _gppy was built with matching pybind11.");
  }
}

/// @brief Cast a C++ Circuit to a Python stim.Circuit.
inline auto circuitToPython(stim::Circuit circuit) -> pybind11::object {
  ensureStimImported();
  try {
    return pybind11::cast(std::move(circuit));
  } catch (const pybind11::cast_error &) {
    throw std::runtime_error(
        "Failed to cast stim.Circuit to Python. "
        "Ensure stim is installed and _gppy was built with matching pybind11.");
  }
}

/// @brief Cast a C++ DetectorErrorModel to a Python stim.DetectorErrorModel.
inline auto demToPython(stim::DetectorErrorModel dem) -> pybind11::object {
  ensureStimImported();
  try {
    return pybind11::cast(std::move(dem));
  } catch (const pybind11::cast_error &) {
    throw std::runtime_error(
        "Failed to cast stim::DetectorErrorModel to Python. "
        "Ensure stim is installed and _gppy was built with matching pybind11.");
  }
}

} // namespace gp

#endif // GREENPEAS_QEC_SHIMS_STIM_PYBIND_HPP

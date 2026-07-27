#ifndef GREENPEAS_QEC_SHIMS_STIM_PYBIND_HPP
#define GREENPEAS_QEC_SHIMS_STIM_PYBIND_HPP

#include <cstddef>
#include <stdexcept>
#include <utility>

#include <pybind11/numpy.h>
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

/// @brief Convert a sparse shot to dense arrays of detectors and observables.
/// @param shot Sparse shot.
/// @param numDetectors Number of detectors.
/// @param numObservables Number of observables.
inline auto sparseShotToDenseArrays(const stim::SparseShot &shot,
                                    size_t numDetectors,
                                    size_t numObservables)
    -> std::pair<pybind11::array_t<bool>, pybind11::array_t<bool>> {
  auto detArray = pybind11::array_t<bool>(numDetectors);
  auto obsArray = pybind11::array_t<bool>(numObservables);
  auto *detView = detArray.mutable_data();
  auto *obsView = obsArray.mutable_data();

  for (size_t i = 0; i < numDetectors; ++i) {
    detView[i] = false;
  }

  for (const uint64_t i : shot.hits) {
    detView[i] = true;
  }

  for (size_t i = 0; i < numObservables; ++i) {
    obsView[i] = shot.obs_mask[i];
  }

  return {std::move(detArray), std::move(obsArray)};
}

} // namespace gp

#endif // GREENPEAS_QEC_SHIMS_STIM_PYBIND_HPP

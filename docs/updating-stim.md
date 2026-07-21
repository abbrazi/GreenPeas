# Updating Stim

GreenPeas pins Stim in `pyproject.toml` and `cmake/dependencies.cmake`

Casts across modules via `Stim.pybind.hpp` require `_gppy` and `stim` to share pybind11 internal IDs.

1. Install the target Stim into a venv and read its pybind11 build ABI:

   ```sh
   pip install stim==X.Y.Z
   python scripts/get_stim_pybind11_build_abi.py
   ```

2. Set `PYBIND11_BUILD_ABI` in `cmake/python.cmake` from the script output

3. Align the pybind11 versions in `pyproject.toml` / `cmake/python.cmake` with the version used by Stim X.Y.Z

4. Bump `stim==X.Y.Z` in `pyproject.toml` and `GIT_TAG vX.Y.Z` in `cmake/dependencies.cmake`

5. `pip install .[dev] && pytest`

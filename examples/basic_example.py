"""Compile DEMs for BB code circuits on the GPU (buffer reuse)."""

from __future__ import annotations

from collections.abc import Iterator

import greenpeas as gp
import stim


def get_max_circuit() -> stim.Circuit:
    return gp.BBCode(18).get_memory(rounds=18, p=0.001)


def iter_circuits() -> Iterator[tuple[int, stim.Circuit]]:
    """Yield (distance, circuit) pairs bounded by the max circuit."""
    for distance in (12, 10, 6):
        yield distance, gp.BBCode(distance).get_memory(rounds=distance, p=0.001)


# Construct the max circuit.
max_circuit = get_max_circuit()

# Construct the compiler driver the max circuit.
driver = gp.get_driver(max_circuit)

# Compile the DEM for max circuit.
dem = driver.compile_detector_error_model()
print(f"distance 18: {dem.num_errors} errors")

# Reuse the same buffers for the smaller circuits.
for distance, circuit in iter_circuits():
    dem = driver.compile_detector_error_model(circuit)
    print(f"distance {distance}: {dem.num_errors} errors")

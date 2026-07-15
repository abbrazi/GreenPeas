"""Compile DEMs on the GPU."""

import greenpeas as gp

# Get the largest circuit.
circuit = gp.SurfaceCode(9).get_memory(rounds=9, p=0.001)

# Get a compiler driver for the largest circuit.
driver = gp.get_driver(circuit)

# Compile DEMs for various circuits within the bounds of the largest circuit.
for distance in (9, 7, 5, 3):
    circuit = gp.SurfaceCode(distance).get_memory(rounds=distance, p=0.001)
    dem = driver.compile_detector_error_model(circuit)
    print(f"distance {distance}: {dem.num_errors} errors")

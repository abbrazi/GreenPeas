"""Compile DEMs for BB code circuits on the GPU."""

import greenpeas as gp

# Construct the largest circuit.
largest_circuit = gp.BBCode(18).get_memory(rounds=18, p=0.001)

# Instantiate a compiler driver based on the dimensions of the largest circuit.
driver = gp.get_driver(largest_circuit)

# Compile the DEM for the largest circuit.
dem = driver.compile_detector_error_model(largest_circuit)
print(f"distance 18: {dem.num_errors} errors")

# Compile the DEMs for the smaller circuits.
for distance in (12, 10, 6):
    smaller_circuit = gp.BBCode(distance).get_memory(rounds=distance, p=0.001)
    dem = driver.compile_detector_error_model(smaller_circuit)
    print(f"distance {distance}: {dem.num_errors} errors")

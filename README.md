# GreenPeas

GPU-accelerated compilation of *detector error models (DEMs)* for adaptive quantum error correction.

GreenPeas builds DEMs just-in-time so decoders can track mid-circuit branching and time-varying noise.

It is up to an order of magnitude faster than standard CPU methods (see [paper](https://arxiv.org/abs/2604.16613)).

## Prerequisites

- NVIDIA GPU and a working CUDA toolkit
- CMake ≥ 3.27 and Ninja
- Dev container (recommended): reopen the repo in the GreenPeas container from VS Code

## Quick start

1. Source the setup script with the compute capability `XX` of your GPU card, e.g., `120` for Blackwell:

    ```sh
    source ./scripts/setup.sh XX
    ```

2. Install the `greenpeas` package with the optional dev dependencies:

    ```sh
    python3 -m venv venv
    source venv/bin/activate
    pip install .[dev]
    ```

3. Run the Python unit tests:

    ```sh
    pytest
    ```

## Basic API example

```python
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
```

## Artifact evaluation

To reproduce the paper results, see the [artifact evaluation instructions](ae/README.md).

## Citing GreenPeas

If you use GreenPeas in your research, please cite:

```bibtex
@misc{ziad2026greenpeas,
      title={GreenPeas: Unlocking adaptive quantum error correction with just-in-time decoding hypergraphs}, 
      author={Abbas B. Ziad and Jubo Xu and Hongxiang Fan},
      year={2026},
      eprint={2604.16613},
      archivePrefix={arXiv},
      primaryClass={quant-ph},
      url={https://arxiv.org/abs/2604.16613}, 
}
```

## Contact

For any questions or concerns, please [email me](mailto:abbasbrackenziad@gmail.com).

## License

Copyright 2026 Abbas Bracken Ziad

Licensed under the [Apache License 2.0](LICENSE).

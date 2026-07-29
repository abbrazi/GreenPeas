# Artifact evaluation

Reproduce the paper figures with the GreenPeas CLI.

## Prerequisites

- NVIDIA GPU and a working CUDA toolkit
- CMake ≥ 3.27 and Ninja
- Dev container (recommended): reopen the repo in the GreenPeas container from VS Code

## Build the CLI

1. From the repository root, source the setup script with your GPU compute capability (`XX`), e.g. `86` for Ampere:

   ```sh
   source ./scripts/setup.sh XX
   ```

   This exports `CMAKE_ARGS` with `-DGP_USE_CUDA=ON` and `-DGP_CUDA_ARCHITECTURES=XX`.

2. Configure and build:

   ```sh
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release $CMAKE_ARGS
   cmake --build build --target gp
   ```

## Run the CLI

List commands:

```sh
./build/gp --help
```

Subcommands (stubs until result generation is wired up):

| Command | Figure |
| --- | --- |
| `gen-static-memory-compilation-results` | Fig. 3 |
| `gen-static-memory-decoding-results` | Fig. 4 |
| `gen-adaptive-memory-compilation-results` | Fig. 6 |
| `gen-adaptive-memory-decoding-results` | Fig. 7 |

Example:

```sh
./build/gp gen-static-memory-compilation-results -n 1000
```

Each subcommand writes a CSV header under `data/results/`:

| Command | Output |
| --- | --- |
| `gen-static-memory-compilation-results` | `data/results/static_memory_compilation.csv` |
| `gen-static-memory-decoding-results` | `data/results/static_memory_decoding.csv` |
| `gen-adaptive-memory-compilation-results` | `data/results/adaptive_memory_compilation.csv` |
| `gen-adaptive-memory-decoding-results` | `data/results/adaptive_memory_decoding.csv` |

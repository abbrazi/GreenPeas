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

Subcommands:

| Command | Figure |
| --- | --- |
| `single-level-compile` | Fig. 3 |
| `single-level-decode` | Fig. 4 |
| `multi-level-compile` | Fig. 6 |
| `multi-level-decode` | Fig. 7 |

Example:

```sh
./build/gp single-level-compile -n 1000
```

Each subcommand writes a CSV under `data/ae/`:

| Command | Output |
| --- | --- |
| `single-level-compile` | `data/ae/single_level_compilation.csv` |
| `single-level-decode` | `data/ae/single_level_decoding.csv` |
| `multi-level-compile` | `data/ae/multi_level_compilation.csv` |
| `multi-level-decode` | `data/ae/multi_level_decoding.csv` |

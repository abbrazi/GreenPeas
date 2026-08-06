# Artifact evaluation

Reproduce the main paper results with the GreenPeas CLI.

## Prerequisites

- NVIDIA Blackwell GPU and a working CUDA toolkit
- CMake ≥ 3.27 and Ninja
- Dev container (recommended): reopen the repo in the GreenPeas container from VS Code

## Build the CLI

1. From the repository root, source the setup script with your GPU compute capability (`XX`), e.g. `120` for Blackwell:

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
| `single-level-compile` | Fig. 3 (a)|
| `single-level-decode` | Fig. 3 (b) and (c) |
| `multi-level-compile` | Fig. 6 (a) |
| `multi-level-decode` | Fig. 6 (b) and (c) |

Example:

```sh
./build/gp single-level-compile -n 10000
```

Each subcommand writes a CSV under `ae/results/`:

| Command | Output |
| --- | --- |
| `single-level-compile` | `ae/results/single_level_compile.csv` |
| `single-level-decode` | `ae/results/single_level_decode.csv` |
| `multi-level-compile` | `ae/results/multi_level_compile.csv` |
| `multi-level-decode` | `ae/results/multi_level_decode.csv` |

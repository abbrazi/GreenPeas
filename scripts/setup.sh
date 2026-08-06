#!/usr/bin/env bash
#
# Configure GreenPeas CUDA build.
#
# Usage: source scripts/setup.sh <sm_architectures>

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  echo "error: source this script, e.g. 'source scripts/setup.sh 120'" >&2
  exit 1
fi

sm_architectures="${1:-}"

if [[ -z "${sm_architectures}" ]]; then
  echo "usage: source scripts/setup.sh <sm_architectures>" >&2
  echo "  e.g. source scripts/setup.sh 120   # Blackwell" >&2
  return 1
fi

cmake_args=(
  "-DGP_USE_CUDA=ON"
  "-DGP_CUDA_ARCHITECTURES=${sm_architectures}"
)

export CMAKE_ARGS="${cmake_args[*]}${CMAKE_ARGS:+ ${CMAKE_ARGS}}"

echo "Configured GreenPeas CUDA build for architectures: ${sm_architectures}"

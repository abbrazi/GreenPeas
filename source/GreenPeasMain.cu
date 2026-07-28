/// Standard headers
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <thread>

/// External headers
#include <CLI/CLI.hpp>

/// Project headers
#include "GreenPeas/Cli/Cli.hpp"
#include "GreenPeas/Policies/Compute/CUDA.hpp"
#include "GreenPeas/Policies/Data/Layout.hpp"
#include "GreenPeas/Policies/Storage/CUDA.hpp"

using namespace gp;

using CUDACli = Cli<CUDAStorage, CUDACompute, ColMajorLayout>;

auto gpMain(int argc, char **argv) -> int {
  CLI::App app{"Evaluate the GreenPeas artifact.", "gp"};
  app.require_subcommand(1);
  app.get_formatter()->column_width(50);

  uint64_t shots = 0;
  size_t threads = std::max<size_t>(1, std::thread::hardware_concurrency());

  auto *staticCompile = app.add_subcommand(
      "static-compile", "Generate static memory compilation results (Fig. 3).");
  staticCompile->add_option("-n,--shots", shots, "Number of shots")->required();

  auto *staticDecode = app.add_subcommand(
      "static-decode", "Generate static memory decoding results (Fig. 4).");
  staticDecode->add_option("-n,--shots", shots, "Number of shots")->required();
  staticDecode->add_option("-j,--threads", threads, "Number of decode threads")
      ->capture_default_str();

  auto *adaptiveCompile = app.add_subcommand(
      "adaptive-compile",
      "Generate adaptive memory compilation results (Fig. 6).");
  adaptiveCompile->add_option("-n,--shots", shots, "Number of shots")
      ->required();

  auto *adaptiveDecode = app.add_subcommand(
      "adaptive-decode", "Generate adaptive memory decoding results (Fig. 7).");
  adaptiveDecode->add_option("-n,--shots", shots, "Number of shots")
      ->required();

  CLI11_PARSE(app, argc, argv);

  if (*staticCompile) {
    return CUDACli::staticCompile(shots);
  }
  if (*staticDecode) {
    return CUDACli::staticDecode(shots, threads);
  }
  if (*adaptiveCompile) {
    return CUDACli::adaptiveCompile(shots);
  }
  if (*adaptiveDecode) {
    return CUDACli::adaptiveDecode(shots);
  }

  std::cerr << "No matching subcommand.\n";
  return 1;
}

/// Standard headers
#include <algorithm>
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

  size_t shots = 0;
  size_t threads = std::max<size_t>(1, std::thread::hardware_concurrency());

  auto *singleLevelCompile = app.add_subcommand(
      "single-level-compile",
      "Generate compilation results for single-level codes.");
  singleLevelCompile->add_option("-n,--shots", shots, "Number of shots")
      ->required();

  auto *singleLevelDecode = app.add_subcommand(
      "single-level-decode",
      "Generate decoding results for single-level codes.");
  singleLevelDecode->add_option("-n,--shots", shots, "Number of shots")
      ->required();
  singleLevelDecode
      ->add_option("-j,--threads", threads, "Number of decode threads")
      ->capture_default_str();

  auto *multiLevelCompile = app.add_subcommand(
      "multi-level-compile",
      "Generate compilation results for multi-level codes.");
  multiLevelCompile->add_option("-n,--shots", shots, "Number of shots")
      ->required();

  auto *multiLevelDecode = app.add_subcommand(
      "multi-level-decode",
      "Generate decoding results for multi-level codes.");
  multiLevelDecode->add_option("-n,--shots", shots, "Number of shots")
      ->required();
  multiLevelDecode
      ->add_option("-j,--threads", threads, "Number of decode threads")
      ->capture_default_str();

  CLI11_PARSE(app, argc, argv);

  if (*singleLevelCompile) {
    return CUDACli::singleLevelCompile(shots);
  }
  if (*singleLevelDecode) {
    return CUDACli::singleLevelDecode(shots, threads);
  }
  if (*multiLevelCompile) {
    return CUDACli::multiLevelCompile(shots);
  }
  if (*multiLevelDecode) {
    return CUDACli::multiLevelDecode(shots, threads);
  }

  std::cerr << "No matching subcommand.\n";
  return 1;
}

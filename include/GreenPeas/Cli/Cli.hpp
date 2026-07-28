#ifndef GREENPEAS_CLI_CLI_HPP
#define GREENPEAS_CLI_CLI_HPP

/// Standard headers
#include <atomic>
#include <chrono>
#include <cstdint>
#include <ostream>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

/// Project headers
#include "GreenPeas/Cli/IO.hpp"
#include "GreenPeas/Common.hpp"
#include "GreenPeas/Perf/Timing.hpp"
#include "GreenPeas/QEC/Codes/Code.hpp"
#include "GreenPeas/QEC/ErrorAnalysis/Driver.hpp"

/// External headers
#include "stim.h"
#include "tesseract.h"

namespace gp {

/// @brief Command-line entry points for GreenPeas artifact evaluation.
template <typename Storage, typename Compute, typename Layout>
struct Cli {
  /// == Aliases ===============================================================

  template <CorrelationLevel Level>
  using CompilerDriver = Driver<Storage, Compute, Layout, Level>;

  using Duration = std::chrono::microseconds;

  using DEM = stim::DetectorErrorModel;

  using Shots = std::vector<stim::SparseShot>;
  using Preds = std::vector<std::vector<int>>;
  using Times = std::vector<Duration>;

  // == Static compile types ===================================================

  /// @brief Static compile output.
  struct SCO {
    TimingStats timing{};
  };

  /// @brief Static compile row.
  struct SCR {
    uint64_t n{};
    std::string code;
    SCO gp0{};
    SCO gp1{};
    SCO gp2{};
    SCO ref{};
  };

  /// @brief Serialize a static-compile CSV row.
  HOST static auto stringOf(const SCR &r) -> std::string {
    std::ostringstream out;
    out << r.n << ',' << r.code << ',' << r.gp0.timing << ',' << r.gp1.timing
        << ',' << r.gp2.timing << ',' << r.ref.timing;
    return out.str();
  }

  // == Static decode types ===================================================

  /// @brief Static decode output.
  struct SDO {
    size_t fails{};
    TimingStats timing{};
  };

  /// @brief Static decode row.
  struct SDR {
    uint64_t n{};
    std::string code;
    SDO gp0{};
    SDO gp1{};
    SDO gp2{};
    SDO ref{};
  };

  /// @brief Serialize a static-decode CSV row.
  HOST static auto stringOf(const SDR &r) -> std::string {
    std::ostringstream out;
    out << r.n << ',' << r.code << ',' << r.gp0.fails << ',' << r.gp0.timing
        << ',' << r.gp1.fails << ',' << r.gp1.timing << ',' << r.gp2.fails
        << ',' << r.gp2.timing << ',' << r.ref.fails << ',' << r.ref.timing;
    return out.str();
  }

  // == Entry points ==========================================================

  /// @brief Generate static memory compilation results (Fig. 3).
  HOST static auto staticCompile(uint64_t n) -> int {
    log("Generating static memory compilation results");
    log("Running n = " + std::to_string(n) + " shots");

    constexpr double p = 0.001;
    constexpr std::string_view filename = "static_memory_compilation.csv";

    writeResultsCsvHeader(filename,
                          "n,code,"
                          "gp0_tavg,gp0_tstd,"
                          "gp1_tavg,gp1_tstd,"
                          "gp2_tavg,gp2_tstd,"
                          "ref_tavg,ref_tstd");

    for (const uint32_t d : {3u, 5u, 7u, 9u}) {
      const SurfaceCode code(d, GP_DATA_PATH);
      const stim::Circuit circuit = code.getMemory(d, p);
      runStaticCompile(n, "surface" + std::to_string(d), circuit, filename);
    }

    for (const uint32_t d : {6u, 10u, 12u}) {
      const BBCode code(d, GP_DATA_PATH);
      const stim::Circuit circuit = code.getMemory(d, p);
      runStaticCompile(n, "bb" + std::to_string(d), circuit, filename);
    }

    log("Wrote " + csvPath(filename).string());
    return 0;
  }

  /// @brief Generate static memory decoding results (Fig. 4).
  HOST static auto staticDecode(uint64_t n, size_t j) -> int {
    log("Generating static memory decoding results");
    log("Running n = " + std::to_string(n) + " shots");
    log("Using j = " + std::to_string(j) + " threads");

    constexpr double p = 0.001;
    constexpr std::string_view filename = "static_memory_decoding.csv";

    writeResultsCsvHeader(filename,
                          "n,code,"
                          "gp0_fails,gp0_tavg,gp0_tstd,"
                          "gp1_fails,gp1_tavg,gp1_tstd,"
                          "gp2_fails,gp2_tavg,gp2_tstd,"
                          "ref_fails,ref_tavg,ref_tstd");

    for (const uint32_t d : {3u, 5u, 7u, 9u}) {
      const SurfaceCode code(d, GP_DATA_PATH);
      const stim::Circuit circuit = code.getMemory(d, p);
      runStaticDecode(
          n, "surface" + std::to_string(d), d, circuit, filename, j);
    }

    for (const uint32_t d : {6u, 10u, 12u}) {
      const BBCode code(d, GP_DATA_PATH);
      const stim::Circuit circuit = code.getMemory(d, p);
      runStaticDecode(n, "bb" + std::to_string(d), d, circuit, filename, j);
    }

    log("Wrote " + csvPath(filename).string());
    return 0;
  }

  /// @brief Generate adaptive memory compilation results (Fig. 6).
  HOST static auto adaptiveCompile(uint64_t n) -> int {
    log("Generating adaptive memory compilation results");
    log("Running n = " + std::to_string(n) + " shots");

    writeResultsCsvHeader("adaptive_memory_compilation.csv",
                          "n,strategy,"
                          "phe_tavg,phe_tstd,"
                          "gp0_tavg,gp0_tstd,"
                          "gp1_tavg,gp1_tstd,"
                          "gp2_tavg,gp2_tstd,"
                          "ref_tavg,ref_tstd");

    log("Wrote " + csvPath("adaptive_memory_compilation.csv").string());
    return 0;
  }

  /// @brief Generate adaptive memory decoding results (Fig. 7).
  HOST static auto adaptiveDecode(uint64_t n) -> int {
    log("Generating adaptive memory decoding results");
    log("Running n = " + std::to_string(n) + " shots");

    writeResultsCsvHeader("adaptive_memory_decoding.csv",
                          "n,strategy,"
                          "gp0_fails,gp0_tavg,gp0_tstd,"
                          "phe_fails,phe_tavg,phe_tstd");
    log("Wrote " + csvPath("adaptive_memory_decoding.csv").string());
    return 0;
  }

  // == Compile helpers =======================================================

  /// @brief Compile a detector error model with Stim.
  HOST static auto compileWithStim(const stim::Circuit &stimCircuit) -> DEM {
    return stim::ErrorAnalyzer::circuit_to_detector_error_model(
        stimCircuit,
        false, // decompose_errors
        false, // fold_loops
        false, // allow_gauge_detectors
        0.0,   // approximate_disjoint_errors_threshold
        false, // ignore_decomposition_failures
        true); // block_decomposition_from_introducing_remnant_edges
  }

  /// @brief Compile DEM for @p circuit with GreenPeas @p n times.
  template <CorrelationLevel Level>
  HOST static auto compileDEM(const stim::Circuit &circuit, uint64_t n) -> SCO {
    auto driver = CompilerDriver<Level>::fromStimCircuit(circuit);

    // warmup
    (void)driver.compileDetectorErrorModel(circuit);

    return SCO{time<Duration>(static_cast<size_t>(n), true, [&] {
      (void)driver.compileDetectorErrorModel(circuit);
    })};
  }

  /// @brief Compile DEM for @p circuit with Stim @p n times.
  HOST static auto compileREF(const stim::Circuit &circuit, uint64_t n) -> SCO {
    // warmup
    (void)compileWithStim(circuit);

    return SCO{time<Duration>(
        static_cast<size_t>(n), true, [&] { (void)compileWithStim(circuit); })};
  }

  /// @brief Time GreenPeas L0–L2 and Stim DEM compile for one code; append CSV.
  HOST static void runStaticCompile(uint64_t n,
                                    std::string_view code,
                                    const stim::Circuit &circuit,
                                    std::string_view filename) {
    log("Evaluating " + std::string(code));
    SCR row;
    row.n = n;
    row.code = std::string(code);
    row.gp0 = compileDEM<CorrelationLevel::L0>(circuit, n);
    row.gp1 = compileDEM<CorrelationLevel::L1>(circuit, n);
    row.gp2 = compileDEM<CorrelationLevel::L2>(circuit, n);
    row.ref = compileREF(circuit, n);
    appendResultsCsvRow(filename, stringOf(row));
  }

  // == Decode helpers ========================================================

  /// @brief Sample @p n sparse detection-event shots from @p stimCircuit.
  HOST static auto sample(const stim::Circuit &stimCircuit, size_t n) -> Shots {
    auto rng = stim::externally_seeded_rng();

    const auto [dets, obss] =
        stim::sample_batch_detection_events<64>(stimCircuit, n, rng);

    const auto obssT = obss.transposed();
    const auto detsT = dets.transposed();

    Shots shots(n);
    for (size_t k = 0; k < n; k++) {
      shots[k].obs_mask = obssT[k];
      auto row = detsT[k];
      row.for_each_set_bit([&](size_t d) { shots[k].hits.push_back(d); });
    }
    return shots;
  }

  /// @brief Build the short-beam Tesseract decoder config.
  HOST static auto tesseractConfig(const DEM &dem) -> TesseractConfig {
    TesseractConfig config{dem};
    config.pqlimit = 200'000;
    config.det_beam = 15;
    config.beam_climbing = true;
    config.no_revisit_dets = true;
    config.det_orders = build_det_orders(dem, 16, DetOrder::DetIndex, 0);
    return config;
  }

  /// @brief Decode @p shots with up to @p j threads.
  HOST static auto decodeShots(const TesseractConfig &config,
                               const Shots &shots,
                               size_t j) -> std::pair<Preds, Times> {
    Preds preds(shots.size());
    Times times(shots.size());

    const auto nThreads = std::max<size_t>(1, std::min(j, shots.size()));
    std::vector<std::thread> threads;
    threads.reserve(nThreads);

    std::atomic<size_t> shot{0};

    for (size_t t = 0; t < nThreads; ++t) {
      threads.emplace_back([&] {
        TesseractDecoder decoder(config);
        for (size_t i; (i = shot++) < shots.size();) {
          times[i] = timeOnce<Duration>([&] {
            decoder.decode_to_errors(shots[i].hits);
            auto &predictedErrors = decoder.predicted_errors_buffer;
            preds[i] = decoder.get_flipped_observables(predictedErrors);
          });
        }
      });
    }

    for (auto &th : threads) {
      th.join();
    }

    return {std::move(preds), std::move(times)};
  }

  /// @brief Count shots whose predicted observables disagree with the obs mask.
  HOST static auto fails(const Shots &shots, const Preds &preds) -> size_t {
    size_t nFails = 0;
    for (size_t i = 0; i < shots.size(); ++i) {
      if (vector_to_u64_mask(preds[i]) != shots[i].obs_mask_as_u64()) {
        ++nFails;
      }
    }
    return nFails;
  }

  /// @brief Mean and std-dev of per-shot decode times.
  HOST static auto timingStats(const Times &times) -> TimingStats {
    const double avgShot = computeAvg(times);
    const double stdShot = computeStd(times, avgShot);
    return TimingStats{avgShot, stdShot};
  }

  /// @brief Decode @p shots under @p dem with @p j threads.
  HOST static auto decode(const DEM &dem, const Shots &shots, size_t j) -> SDO {
    const auto config = tesseractConfig(dem);
    const auto [preds, times] = decodeShots(config, shots, j);
    return SDO{fails(shots, preds), timingStats(times)};
  }

  /// @brief Decode one code with GreenPeas L0–L2 and Stim DEMs.
  HOST static void runStaticDecode(uint64_t n,
                                   std::string_view code,
                                   uint32_t d,
                                   const stim::Circuit &circuit,
                                   std::string_view filename,
                                   size_t j) {
    log("Evaluating " + std::string(code));

    const auto shots = sample(circuit, static_cast<size_t>(n));

    using Driver0 = Driver<Storage, Compute, Layout, CorrelationLevel::L0>;
    using Driver1 = Driver<Storage, Compute, Layout, CorrelationLevel::L1>;
    using Driver2 = Driver<Storage, Compute, Layout, CorrelationLevel::L2>;

    auto driver0 = Driver0::fromStimCircuit(circuit);
    auto driver1 = Driver1::fromStimCircuit(circuit);
    auto driver2 = Driver2::fromStimCircuit(circuit);

    SDR row;
    row.n = n;
    row.code = std::string(code);
    row.gp0 = decode(driver0.compileDetectorErrorModel(circuit), shots, j);
    row.gp1 = decode(driver1.compileDetectorErrorModel(circuit), shots, j);
    row.gp2 = decode(driver2.compileDetectorErrorModel(circuit), shots, j);
    row.ref = decode(compileWithStim(circuit), shots, j);

    appendResultsCsvRow(filename, stringOf(row));
  }
};

} // namespace gp

#endif // GREENPEAS_CLI_CLI_HPP

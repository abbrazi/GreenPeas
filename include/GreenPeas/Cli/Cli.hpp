#ifndef GREENPEAS_CLI_CLI_HPP
#define GREENPEAS_CLI_CLI_HPP

/// Standard headers
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iterator>
#include <ostream>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

/// Project headers
#include "GreenPeas/Cli/IO.hpp"
#include "GreenPeas/Common.hpp"
#include "GreenPeas/Perf/Timing.hpp"
#include "GreenPeas/QEC/Codes/Code.hpp"
#include "GreenPeas/QEC/Codes/ConcatenatedSurfaceCode.hpp"
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

  // == Single-level compile types ============================================

  /// @brief Compilation output for a single-level code.
  struct SLCO {
    TimingStats timing{};
  };

  /// @brief Compilation row for a single-level code.
  struct SLCR {
    size_t n{};
    std::string code;
    SLCO gp0{};
    SLCO gp1{};
    SLCO gp2{};
    SLCO ref{};
  };

  /// @brief Serialise an SLCR.
  HOST static auto stringOf(const SLCR &r) -> std::string {
    std::ostringstream out;
    out << r.n << ',' << r.code << ',' << r.gp0.timing << ',' << r.gp1.timing
        << ',' << r.gp2.timing << ',' << r.ref.timing;
    return out.str();
  }

  // == Single-level decode types =============================================

  /// @brief Decode output for a single-level code.
  struct SLDO {
    size_t fails{};
    TimingStats timing{};
  };

  /// @brief Decode row for a single code.
  struct SLDR {
    size_t n{};
    std::string code;
    SLDO gp0{};
    SLDO gp1{};
    SLDO gp2{};
    SLDO ref{};
  };

  /// @brief Serialise an SLDR.
  HOST static auto stringOf(const SLDR &r) -> std::string {
    std::ostringstream out;
    out << r.n << ',' << r.code << ',' << r.gp0.fails << ',' << r.gp0.timing
        << ',' << r.gp1.fails << ',' << r.gp1.timing << ',' << r.gp2.fails
        << ',' << r.gp2.timing << ',' << r.ref.fails << ',' << r.ref.timing;
    return out.str();
  }

  // == Multi-level compile types =============================================

  /// @brief Compile output for a multi-level code.
  using MLCO = SLCO;

  /// @brief Compile row for a multi-level code.
  struct MLCR {
    size_t n{};
    std::string code;
    std::string strategy;
    MLCO phe{};
    MLCO gp0{};
    MLCO gp1{};
    MLCO gp2{};
    MLCO ref{};
  };

  /// @brief Serialise an MLCR.
  HOST static auto stringOf(const MLCR &r) -> std::string {
    std::ostringstream out;
    out << r.n << ',' << r.code << ',' << r.strategy << ',' << r.phe.timing
        << ',' << r.gp0.timing << ',' << r.gp1.timing << ',' << r.gp2.timing
        << ',' << r.ref.timing;
    return out.str();
  }

  // == Multi-level decode types ==============================================

  /// @brief Decode output for a multi-level code.
  using MLDO = SLDO;

  /// @brief Decode row for a multi-level code.
  struct MLDR {
    size_t n{};
    std::string code;
    std::string strategy;
    MLDO gp0{};
    MLDO phe{};
  };

  /// @brief Serialise an MLDR.
  HOST static auto stringOf(const MLDR &r) -> std::string {
    std::ostringstream out;
    out << r.n << ',' << r.code << ',' << r.strategy << ',' << r.phe.fails
        << ',' << r.phe.timing << ',' << r.gp0.fails << ',' << r.gp0.timing;
    return out.str();
  }

  // == Entry points ==========================================================

  /// @brief Generate compilation results for single-level codes (Fig. 3).
  HOST static auto singleLevelCompile(size_t n) -> int {
    log("Generating compilation results for single-level codes");
    log("Running n = " + std::to_string(n) + " shots");

    constexpr double p = 0.001;
    constexpr std::string_view filename = "single_level_compile.csv";

    writeResultsCsvHeader(filename,
                          "n,code,"
                          "gp0_tavg,gp0_tstd,"
                          "gp1_tavg,gp1_tstd,"
                          "gp2_tavg,gp2_tstd,"
                          "ref_tavg,ref_tstd");

    for (const uint32_t d : {3u, 5u, 7u, 9u}) {
      const SurfaceCode code(d, GP_DATA_PATH);
      // L0 Z-basis experiments exclude X-detectors.
      const stim::Circuit circuitNoX =
          code.getMemory(d, p, /*includeXDetectors=*/false);
      const stim::Circuit circuit =
          code.getMemory(d, p, /*includeXDetectors=*/true);
      slCompile(
          n, "surface" + std::to_string(d), circuitNoX, circuit, filename);
    }

    for (const uint32_t d : {6u, 10u, 12u}) {
      const BBCode code(d, GP_DATA_PATH);
      const stim::Circuit circuitNoX =
          code.getMemory(d, p, /*includeXDetectors=*/false);
      const stim::Circuit circuit =
          code.getMemory(d, p, /*includeXDetectors=*/true);
      slCompile(n, "bb" + std::to_string(d), circuitNoX, circuit, filename);
    }

    log("Wrote " + csvPath(filename).string());
    return 0;
  }

  /// @brief Generate decoding results for single-level codes (Fig. 4).
  HOST static auto singleLevelDecode(size_t n, size_t j) -> int {
    log("Generating decoding results for single-level codes");
    log("Running n = " + std::to_string(n) + " shots");
    log("Using j = " + std::to_string(j) + " threads");

    constexpr double p = 0.001;
    constexpr std::string_view filename = "single_level_decode.csv";

    writeResultsCsvHeader(filename,
                          "n,code,"
                          "gp0_fails,gp0_tavg,gp0_tstd,"
                          "gp1_fails,gp1_tavg,gp1_tstd,"
                          "gp2_fails,gp2_tavg,gp2_tstd,"
                          "ref_fails,ref_tavg,ref_tstd");

    for (const uint32_t d : {3u, 5u, 7u, 9u}) {
      const SurfaceCode code(d, GP_DATA_PATH);
      const stim::Circuit circuitNoX =
          code.getMemory(d, p, /*includeXDetectors=*/false);
      const stim::Circuit circuit =
          code.getMemory(d, p, /*includeXDetectors=*/true);
      slDecode(
          n, "surface" + std::to_string(d), circuitNoX, circuit, filename, j);
    }

    for (const uint32_t d : {6u, 10u, 12u}) {
      const BBCode code(d, GP_DATA_PATH);
      const stim::Circuit circuitNoX =
          code.getMemory(d, p, /*includeXDetectors=*/false);
      const stim::Circuit circuit =
          code.getMemory(d, p, /*includeXDetectors=*/true);
      slDecode(n, "bb" + std::to_string(d), circuitNoX, circuit, filename, j);
    }

    log("Wrote " + csvPath(filename).string());
    return 0;
  }

  /// @brief Generate compilation results for multi-level codes (Fig. 6).
  HOST static auto multiLevelCompile(size_t n) -> int {
    log("Generating compilation results for multi-level codes");
    log("Running n = " + std::to_string(n) + " shots");

    constexpr double p = 0.001;
    constexpr std::string_view filename = "multi_level_compile.csv";

    writeResultsCsvHeader(filename,
                          "n,code,strategy,"
                          "phe_tavg,phe_tstd,"
                          "gp0_tavg,gp0_tstd,"
                          "gp1_tavg,gp1_tstd,"
                          "gp2_tavg,gp2_tstd,"
                          "ref_tavg,ref_tstd");

    for (const uint32_t d : {4u, 6u, 8u, 10u}) {
      ConcatenatedSurfaceCode code(d, GP_DATA_PATH);
      mlCompile(n, "csc" + std::to_string(d), code, d, p, filename);
    }

    log("Wrote " + csvPath(filename).string());
    return 0;
  }

  /// @brief Generate decoding results for multi-level codes (Fig. 7).
  HOST static auto multiLevelDecode(size_t n, size_t j) -> int {
    log("Generating decoding results for multi-level codes");
    log("Running n = " + std::to_string(n) + " shots");
    log("Using j = " + std::to_string(j) + " threads");

    constexpr double p = 0.001;
    constexpr std::string_view filename = "multi_level_decode.csv";

    writeResultsCsvHeader(filename,
                          "n,code,strategy,"
                          "phe_fails,phe_tavg,phe_tstd,"
                          "gp0_fails,gp0_tavg,gp0_tstd");

    for (const uint32_t d : {4u, 6u, 8u, 10u}) {
      ConcatenatedSurfaceCode code(d, GP_DATA_PATH);
      mlDecode(n, "csc" + std::to_string(d), code, d, p, filename, j);
    }

    log("Wrote " + csvPath(filename).string());
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
  HOST static auto compileDEM(const stim::Circuit &circuit, size_t n) -> SLCO {
    auto driver = CompilerDriver<Level>::fromStimCircuit(circuit);

    // warmup
    (void)driver.compileDetectorErrorModel();

    return SLCO{time<Duration>(
        n, true, [&] { (void)driver.compileDetectorErrorModel(); })};
  }

  /// @brief Compile DEM for @p circuit with Stim @p n times.
  HOST static auto compileREF(const stim::Circuit &circuit, size_t n) -> SLCO {
    // warmup
    (void)compileWithStim(circuit);

    return SLCO{
        time<Duration>(n, true, [&] { (void)compileWithStim(circuit); })};
  }

  /// @brief Compile paths for one single-level code.
  /// @param circuitNoX Z-basis circuit without X-detectors (for L0).
  /// @param circuit Full circuit with X-detectors (for L1/L2/Stim).
  HOST static void slCompile(size_t n,
                             std::string_view code,
                             const stim::Circuit &circuitNoX,
                             const stim::Circuit &circuit,
                             std::string_view filename) {
    log("Evaluating " + std::string(code));
    SLCR row;
    row.n = n;
    row.code = std::string(code);
    row.gp0 = compileDEM<CorrelationLevel::L0>(circuitNoX, n);
    row.gp1 = compileDEM<CorrelationLevel::L1>(circuit, n);
    row.gp2 = compileDEM<CorrelationLevel::L2>(circuit, n);
    row.ref = compileREF(circuit, n);
    appendResultsCsvRow(filename, stringOf(row));
  }

  /// @brief Compile paths for one multi-level code.
  HOST static void mlCompile(size_t n,
                             std::string_view code,
                             ConcatenatedSurfaceCode &csc,
                             uint32_t d,
                             double p,
                             std::string_view filename) {
    // L0 Z-basis experiments exclude X-detectors.
    auto [maxPairNoX, maxShotNoX] = csc.getMemory(
        d, p, MeasurementStrategy::Static, /*includeXDetectors=*/false);
    auto [maxPair, maxShot] = csc.getMemory(
        d, p, MeasurementStrategy::Static, /*includeXDetectors=*/true);
    (void)maxShotNoX;
    (void)maxShot;

    // Drivers constructed once from max circuits; reused for static + adaptive.
    auto driver0 =
        CompilerDriver<CorrelationLevel::L0>::fromStimCircuit(maxPairNoX.circl);
    auto driver1 =
        CompilerDriver<CorrelationLevel::L1>::fromStimCircuit(maxPair.circl);
    auto driver2 =
        CompilerDriver<CorrelationLevel::L2>::fromStimCircuit(maxPair.circl);
    (void)driver0.compileDetectorErrorModel();
    (void)driver1.compileDetectorErrorModel();
    (void)driver2.compileDetectorErrorModel();
    (void)compileWithStim(maxPairNoX.pheno);
    (void)compileWithStim(maxPair.circl);

    {
      log("Evaluating " + std::string(code) + "/static");
      MLCR row;
      row.n = n;
      row.code = std::string(code);
      row.strategy = "static";
      // phe: Stim on phenomenological circuit (traditional toolchain baseline)
      row.phe = SLCO{time<Duration>(
          n, true, [&] { (void)compileWithStim(maxPairNoX.pheno); })};
      row.gp0 = SLCO{time<Duration>(
          n, true, [&] { (void)driver0.compileDetectorErrorModel(); })};
      row.gp1 = SLCO{time<Duration>(
          n, true, [&] { (void)driver1.compileDetectorErrorModel(); })};
      row.gp2 = SLCO{time<Duration>(
          n, true, [&] { (void)driver2.compileDetectorErrorModel(); })};
      row.ref = SLCO{time<Duration>(
          n, true, [&] { (void)compileWithStim(maxPair.circl); })};
      appendResultsCsvRow(filename, stringOf(row));
    }

    {
      log("Evaluating " + std::string(code) + "/adaptive");
      Times pheTimes;
      Times gp0Times;
      Times gp1Times;
      Times gp2Times;
      Times refTimes;
      pheTimes.reserve(n);
      gp0Times.reserve(n);
      gp1Times.reserve(n);
      gp2Times.reserve(n);
      refTimes.reserve(n);

      CircuitPair pairNoX;
      CircuitPair pair;
      stim::SparseShot shotNoX;
      stim::SparseShot shot;

      for (size_t i = 0; i < n; ++i) {
        std::tie(pairNoX, shotNoX) = csc.getMemory(
            d, p, MeasurementStrategy::Adaptive, /*includeXDetectors=*/false);
        std::tie(pair, shot) = csc.getMemory(
            d, p, MeasurementStrategy::Adaptive, /*includeXDetectors=*/true);
        (void)shotNoX;
        (void)shot;

        pheTimes.push_back(
            timeOnce<Duration>([&] { (void)compileWithStim(pairNoX.pheno); }));
        driver0.circuit.parseFromStimCircuit(pairNoX.circl);
        gp0Times.push_back(timeOnce<Duration>(
            [&] { (void)driver0.compileDetectorErrorModel(); }));
        driver1.circuit.parseFromStimCircuit(pair.circl);
        gp1Times.push_back(timeOnce<Duration>(
            [&] { (void)driver1.compileDetectorErrorModel(); }));
        driver2.circuit.parseFromStimCircuit(pair.circl);
        gp2Times.push_back(timeOnce<Duration>(
            [&] { (void)driver2.compileDetectorErrorModel(); }));
        refTimes.push_back(
            timeOnce<Duration>([&] { (void)compileWithStim(pair.circl); }));
      }

      MLCR row;
      row.n = n;
      row.code = std::string(code);
      row.strategy = "adaptive";
      row.phe = MLCO{timingStats(pheTimes)};
      row.gp0 = MLCO{timingStats(gp0Times)};
      row.gp1 = MLCO{timingStats(gp1Times)};
      row.gp2 = MLCO{timingStats(gp2Times)};
      row.ref = MLCO{timingStats(refTimes)};
      appendResultsCsvRow(filename, stringOf(row));
    }
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

  /// @brief Decode @p shots with per-shot configs and up to @p j threads.
  HOST static auto decodeShots(const std::vector<TesseractConfig> &configs,
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
        for (size_t i; (i = shot++) < shots.size();) {
          TesseractDecoder decoder(configs[i]);
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
  HOST static auto decode(const DEM &dem, const Shots &shots, size_t j)
      -> SLDO {
    const auto config = tesseractConfig(dem);
    const auto [preds, times] = decodeShots(config, shots, j);
    return SLDO{fails(shots, preds), timingStats(times)};
  }

  /// @brief Decode @p shots under @p dem and append preds/times.
  HOST static void decode(const DEM &dem,
                          const Shots &shots,
                          size_t j,
                          Preds &allPreds,
                          Times &allTimes) {
    const auto config = tesseractConfig(dem);
    auto [preds, times] = decodeShots(config, shots, j);
    allPreds.insert(allPreds.end(),
                    std::make_move_iterator(preds.begin()),
                    std::make_move_iterator(preds.end()));
    allTimes.insert(allTimes.end(),
                    std::make_move_iterator(times.begin()),
                    std::make_move_iterator(times.end()));
  }

  /// @brief Decode @p shots under per-shot @p dems and append preds/times.
  HOST static void decode(const std::vector<DEM> &dems,
                          const Shots &shots,
                          size_t j,
                          Preds &allPreds,
                          Times &allTimes) {
    std::vector<TesseractConfig> configs;
    configs.reserve(dems.size());
    for (const auto &dem : dems) {
      configs.push_back(tesseractConfig(dem));
    }

    auto [preds, times] = decodeShots(configs, shots, j);
    allPreds.insert(allPreds.end(),
                    std::make_move_iterator(preds.begin()),
                    std::make_move_iterator(preds.end()));
    allTimes.insert(allTimes.end(),
                    std::make_move_iterator(times.begin()),
                    std::make_move_iterator(times.end()));
  }

  /// @brief Decode paths for one single-level code.
  /// @param circuitNoX Z-basis circuit without X-detectors (for L0).
  /// @param circuit Full circuit with X-detectors (for L1/L2/Stim).
  HOST static void slDecode(size_t n,
                            std::string_view code,
                            const stim::Circuit &circuitNoX,
                            const stim::Circuit &circuit,
                            std::string_view filename,
                            size_t j) {
    log("Evaluating " + std::string(code));

    const auto shotsNoX = sample(circuitNoX, n);
    const auto shots = sample(circuit, n);

    using Driver0 = Driver<Storage, Compute, Layout, CorrelationLevel::L0>;
    using Driver1 = Driver<Storage, Compute, Layout, CorrelationLevel::L1>;
    using Driver2 = Driver<Storage, Compute, Layout, CorrelationLevel::L2>;

    auto driver0 = Driver0::fromStimCircuit(circuitNoX);
    auto driver1 = Driver1::fromStimCircuit(circuit);
    auto driver2 = Driver2::fromStimCircuit(circuit);

    SLDR row;
    row.n = n;
    row.code = std::string(code);
    row.gp0 =
        decode(driver0.compileDetectorErrorModel(circuitNoX), shotsNoX, j);
    row.gp1 = decode(driver1.compileDetectorErrorModel(circuit), shots, j);
    row.gp2 = decode(driver2.compileDetectorErrorModel(circuit), shots, j);
    row.ref = decode(compileWithStim(circuit), shots, j);

    appendResultsCsvRow(filename, stringOf(row));
  }

  /// @brief Decode paths for one multi-level code.
  HOST static void mlDecode(size_t n,
                            std::string_view code,
                            ConcatenatedSurfaceCode &csc,
                            uint32_t d,
                            double p,
                            std::string_view filename,
                            size_t j) {
    using Driver0 = Driver<Storage, Compute, Layout, CorrelationLevel::L0>;
    const size_t batchSize = std::max<size_t>(1, j);
    MeasurementStrategy strategy = MeasurementStrategy::Static;

    Shots shots;
    Shots chunk;

    Preds gp0Preds;
    Preds phePreds;
    Times gp0Times;
    Times pheTimes;

    std::vector<DEM> circlDems;
    std::vector<DEM> phenoDems;

    shots.reserve(n);
    chunk.reserve(batchSize);
    gp0Preds.reserve(n);
    phePreds.reserve(n);
    gp0Times.reserve(n);
    pheTimes.reserve(n);

    CircuitPair pair;
    stim::SparseShot shot;

    // L0 Z-basis experiments exclude X-detectors.
    constexpr bool includeXDetectors = false;

    // warmup drivers (constructed once; reused for all DEM compiles)
    std::tie(pair, shot) = csc.getMemory(d, p, strategy, includeXDetectors);
    auto circlDriver = Driver0::fromStimCircuit(pair.circl);
    auto phenoDriver = Driver0::fromStimCircuit(pair.pheno);
    DEM circlDem = circlDriver.compileDetectorErrorModel(pair.circl);
    DEM phenoDem = phenoDriver.compileDetectorErrorModel(pair.pheno);

    {
      log("Evaluating " + std::string(code) + "/static");

      shots.clear();
      gp0Preds.clear();
      phePreds.clear();
      gp0Times.clear();
      pheTimes.clear();

      for (size_t offset = 0; offset < n; offset += batchSize) {
        const size_t chunkSize = std::min(batchSize, n - offset);

        chunk.clear();
        for (size_t i = 0; i < chunkSize; ++i) {
          std::tie(pair, shot) =
              csc.getMemory(d, p, strategy, includeXDetectors);
          chunk.push_back(std::move(shot));
        }

        decode(circlDem, chunk, j, gp0Preds, gp0Times);
        decode(phenoDem, chunk, j, phePreds, pheTimes);
        shots.insert(shots.end(),
                     std::make_move_iterator(chunk.begin()),
                     std::make_move_iterator(chunk.end()));
      }

      MLDR row;
      row.n = n;
      row.code = std::string(code);
      row.strategy = "static";
      row.gp0 = MLDO{fails(shots, gp0Preds), timingStats(gp0Times)};
      row.phe = MLDO{fails(shots, phePreds), timingStats(pheTimes)};
      appendResultsCsvRow(filename, stringOf(row));
    }

    {
      strategy = MeasurementStrategy::Adaptive;
      log("Evaluating " + std::string(code) + "/adaptive");

      shots.clear();
      gp0Preds.clear();
      phePreds.clear();
      gp0Times.clear();
      pheTimes.clear();

      for (size_t offset = 0; offset < n; offset += batchSize) {
        const size_t chunkSize = std::min(batchSize, n - offset);

        chunk.clear();
        circlDems.clear();
        phenoDems.clear();
        for (size_t i = 0; i < chunkSize; ++i) {
          std::tie(pair, shot) =
              csc.getMemory(d, p, strategy, includeXDetectors);
          chunk.push_back(std::move(shot));
          circlDems.push_back(
              circlDriver.compileDetectorErrorModel(pair.circl));
          phenoDems.push_back(
              phenoDriver.compileDetectorErrorModel(pair.pheno));
        }

        decode(circlDems, chunk, j, gp0Preds, gp0Times);
        decode(phenoDems, chunk, j, phePreds, pheTimes);
        shots.insert(shots.end(),
                     std::make_move_iterator(chunk.begin()),
                     std::make_move_iterator(chunk.end()));
      }

      MLDR row;
      row.n = n;
      row.code = std::string(code);
      row.strategy = "adaptive";
      row.gp0 = MLDO{fails(shots, gp0Preds), timingStats(gp0Times)};
      row.phe = MLDO{fails(shots, phePreds), timingStats(pheTimes)};
      appendResultsCsvRow(filename, stringOf(row));
    }
  }
};

} // namespace gp

#endif // GREENPEAS_CLI_CLI_HPP

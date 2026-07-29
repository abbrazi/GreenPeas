#ifndef GREENPEAS_PERF_TIMING_HPP
#define GREENPEAS_PERF_TIMING_HPP

/// Standard headers
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <ostream>
#include <utility>
#include <vector>

namespace gp {

/// @brief Average and sample standard deviation of timings.
struct TimingStats {
  /// @brief Mean of duration counts.
  double avg{};

  /// @brief Sample standard deviation of duration counts.
  double std{};
};

/// @brief Stream `avg,std` for CSV rows.
/// @param os Output stream.
/// @param stats Timing statistics to write.
/// @return @p os.
inline auto operator<<(std::ostream &os, const TimingStats &stats)
    -> std::ostream & {
  return os << stats.avg << ',' << stats.std;
}

/// @brief Compute the average of duration counts.
/// @tparam Duration Input duration type (e.g. `std::chrono::microseconds`).
/// @param times Vector of durations.
/// @return Average of `times` as `double`, or `0` if `times` is empty.
template <typename Duration>
auto computeAvg(const std::vector<Duration> &times) -> double {
  const auto n = times.size();
  if (n == 0) {
    return 0.0;
  }

  double sum = 0.0;
  for (const auto &t : times) {
    sum += static_cast<double>(t.count());
  }
  return sum / static_cast<double>(n);
}

/// @brief Compute the sample standard deviation of duration counts.
/// @tparam Duration Input duration type (e.g. `std::chrono::microseconds`).
/// @param times Vector of durations.
/// @param avg Average of `times` as `double`.
/// @return Sample standard deviation, or `0` if `times.size() <= 1`.
template <typename Duration>
auto computeStd(const std::vector<Duration> &times, double avg) -> double {
  const auto n = times.size();
  if (n <= 1) {
    return 0.0;
  }

  double sumOfSquaredDeviations = 0.0;
  for (const auto &t : times) {
    const double deviation = static_cast<double>(t.count()) - avg;
    sumOfSquaredDeviations += deviation * deviation;
  }

  const double variance = sumOfSquaredDeviations / static_cast<double>(n - 1);
  return std::sqrt(variance);
}

/// @brief Time a single call of a callable.
/// @tparam Duration Result duration type (e.g. `std::chrono::microseconds`).
/// @tparam F Type of the callable.
/// @tparam Args Types of the arguments forwarded to the callable.
/// @param f Callable to run.
/// @param args Arguments to forward to the callable.
/// @return Duration of the call.
template <typename Duration, typename F, typename... Args>
auto timeOnce(F &&f, Args &&...args) -> Duration {
  const auto t0 = std::chrono::steady_clock::now();
  std::forward<F>(f)(std::forward<Args>(args)...);
  const auto t1 = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<Duration>(t1 - t0);
}

/// @brief Time a callable `n` times and summarize the results.
/// @tparam Duration Result duration type (e.g. `std::chrono::microseconds`).
/// @tparam F Type of the callable.
/// @tparam Args Types of the arguments forwarded to the callable.
/// @param n Number of trials.
/// @param dropOutliers If true and `n >= 3`, drop min/max before stats.
/// @param f Callable to run.
/// @param args Arguments to forward to the callable on each trial.
/// @return Average and sample standard deviation of timings.
template <typename Duration, typename F, typename... Args>
auto time(size_t n, bool dropOutliers, F &&f, Args &&...args) -> TimingStats {
  if (n == 0) {
    return TimingStats{};
  }

  std::vector<Duration> times;
  times.reserve(n);
  for (size_t i = 0; i < n; ++i) {
    times.push_back(timeOnce<Duration>(f, args...));
  }

  if (dropOutliers && n >= 3) {
    times.erase(std::min_element(times.begin(), times.end()));
    times.erase(std::max_element(times.begin(), times.end()));
  }

  const double avg = computeAvg(times);
  return TimingStats{avg, computeStd(times, avg)};
}

} // namespace gp

#endif // GREENPEAS_PERF_TIMING_HPP

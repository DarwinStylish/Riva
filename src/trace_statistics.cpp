#include "riva/trace_statistics.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

namespace riva {
namespace {

// Compute the percentile value from a sorted vector using nearest-rank method.
// Percentile is in [0.0, 1.0].
[[nodiscard]] double Percentile(const std::vector<double>& sorted, double percentile) {
  if (sorted.empty()) {
    return 0.0;
  }
  if (sorted.size() == 1) {
    return sorted[0];
  }

  const double rank = percentile * static_cast<double>(sorted.size() - 1);
  const auto lower = static_cast<std::size_t>(std::floor(rank));
  const auto upper = static_cast<std::size_t>(std::ceil(rank));

  if (lower == upper) {
    return sorted[lower];
  }

  const double fraction = rank - static_cast<double>(lower);
  return sorted[lower] * (1.0 - fraction) + sorted[upper] * fraction;
}

// Extract a specific double field from each frame into a sorted vector.
[[nodiscard]] std::vector<double> ExtractSorted(
    const std::vector<Frame>& frames,
    double (*extractor)(const Frame&)) {
  std::vector<double> values;
  values.reserve(frames.size());
  for (const auto& frame : frames) {
    values.push_back(extractor(frame));
  }
  std::sort(values.begin(), values.end());
  return values;
}

}  // namespace

FTraceStatistics ComputeTraceStatistics(
    const NormalizedTrace& trace,
    double hitch_threshold_ms) {
  FTraceStatistics stats;
  const auto& frames = trace.frames();
  stats.total_frames = frames.size();

  if (frames.empty()) {
    return stats;
  }

  // Extract and sort frame durations
  auto durations = ExtractSorted(frames, [](const Frame& f) { return f.duration_ms; });

  stats.p50_ms = Percentile(durations, 0.50);
  stats.p90_ms = Percentile(durations, 0.90);
  stats.p95_ms = Percentile(durations, 0.95);
  stats.p99_ms = Percentile(durations, 0.99);
  stats.min_ms = durations.front();
  stats.max_ms = durations.back();
  stats.mean_ms = std::accumulate(durations.begin(), durations.end(), 0.0)
                  / static_cast<double>(durations.size());

  // Hitch count
  for (double d : durations) {
    if (d > hitch_threshold_ms) {
      ++stats.hitch_count;
    }
  }
  stats.hitch_percentage = static_cast<double>(stats.hitch_count)
                           / static_cast<double>(stats.total_frames) * 100.0;

  // Per-metric P95
  auto game_thread = ExtractSorted(frames, [](const Frame& f) { return f.game_thread_ms; });
  stats.game_thread_p95_ms = Percentile(game_thread, 0.95);

  auto render_thread = ExtractSorted(frames, [](const Frame& f) { return f.render_thread_ms; });
  stats.render_thread_p95_ms = Percentile(render_thread, 0.95);

  auto rhi_thread = ExtractSorted(frames, [](const Frame& f) { return f.rhi_thread_ms; });
  stats.rhi_thread_p95_ms = Percentile(rhi_thread, 0.95);

  auto gpu = ExtractSorted(frames, [](const Frame& f) { return f.gpu_ms; });
  stats.gpu_p95_ms = Percentile(gpu, 0.95);

  auto physics = ExtractSorted(frames, [](const Frame& f) { return f.physics_ms; });
  stats.physics_p95_ms = Percentile(physics, 0.95);

  auto ai = ExtractSorted(frames, [](const Frame& f) { return f.ai_ms; });
  stats.ai_p95_ms = Percentile(ai, 0.95);

  auto network = ExtractSorted(frames, [](const Frame& f) { return f.network_ms; });
  stats.network_p95_ms = Percentile(network, 0.95);

  auto loading = ExtractSorted(frames, [](const Frame& f) { return f.loading_ms; });
  stats.loading_p95_ms = Percentile(loading, 0.95);

  return stats;
}

}  // namespace riva

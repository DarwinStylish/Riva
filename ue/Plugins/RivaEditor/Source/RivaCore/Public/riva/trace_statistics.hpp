#pragma once

#include <cstddef>
#include <string>

#include "riva/export.hpp"
#include "riva/normalized_trace.hpp"

namespace riva {

// Deterministic frame-time percentile statistics computed from a NormalizedTrace.
// All computations use sorted-array percentile selection and are fully
// deterministic across platforms.
struct FTraceStatistics {
  double p50_ms{0.0};
  double p90_ms{0.0};
  double p95_ms{0.0};
  double p99_ms{0.0};
  double min_ms{0.0};
  double max_ms{0.0};
  double mean_ms{0.0};
  std::size_t total_frames{0};
  std::size_t hitch_count{0};
  double hitch_percentage{0.0};

  // Per-metric P95 values
  double game_thread_p95_ms{0.0};
  double render_thread_p95_ms{0.0};
  double rhi_thread_p95_ms{0.0};
  double gpu_p95_ms{0.0};
  double physics_p95_ms{0.0};
  double ai_p95_ms{0.0};
  double network_p95_ms{0.0};
  double loading_p95_ms{0.0};
};

// Compute deterministic trace statistics from a NormalizedTrace.
// hitch_threshold_ms defines the frame duration above which a frame is counted
// as a hitch. Default is 33.333 ms (30 FPS boundary).
[[nodiscard]] RIVACORE_API FTraceStatistics
ComputeTraceStatistics(const NormalizedTrace& trace, double hitch_threshold_ms = 33.333);

}  // namespace riva

#pragma once

#include <string>
#include <vector>

#include "riva/export.hpp"
#include "riva/trace_statistics.hpp"

namespace riva {

// Per-subsystem performance score with human-readable deduction reasons.
struct FSubsystemScore {
  std::string name;
  bool has_data{false};
  double score{0.0};
  std::string grade{"N/A"};
  std::vector<std::string> deductions;
};

// Aggregate performance score spanning all subsystems.
struct FPerformanceScore {
  bool has_data{false};
  double overall{0.0};
  std::string overall_grade{"N/A"};
  std::vector<FSubsystemScore> subsystems;
};

// Configuration for the performance scoring algorithm.
struct FScoreConfig {
  double target_frame_ms{16.667};        // 60 FPS
  double hitch_threshold_ms{33.333};     // 30 FPS
  double max_acceptable_hitch_pct{2.0};  // 2% hitch budget
  double p95_penalty_factor{2.0};        // Penalty multiplier for P95 exceedance
};

// Compute a performance score from trace statistics.
// Returns a 0-100 score with per-subsystem breakdown and letter grades.
[[nodiscard]] RIVACORE_API FPerformanceScore
ComputePerformanceScore(const FTraceStatistics& stats, const FScoreConfig& config = {});

}  // namespace riva

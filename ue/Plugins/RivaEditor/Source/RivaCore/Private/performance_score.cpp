#include "riva/performance_score.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

namespace riva {
namespace {

std::string GradeFromScore(double score) {
  if (score >= 90.0) return "A";
  if (score >= 80.0) return "B";
  if (score >= 70.0) return "C";
  if (score >= 60.0) return "D";
  return "F";
}

std::string FormatMs(double ms) {
  std::ostringstream ss;
  ss << std::fixed << std::setprecision(1) << ms << " ms";
  return ss.str();
}

// Compute a subsystem score from its P95 value against the target.
FSubsystemScore ScoreSubsystem(const std::string& name, double p95_ms, double target_ms,
                               double penalty_factor) {
  FSubsystemScore sub;
  sub.name = name;

  if (p95_ms <= 0.001) {
    return sub;
  }

  sub.has_data = true;
  sub.score = 100.0;

  // Deduct based on how much P95 exceeds the target fraction.
  // Each subsystem's share of the frame budget is its P95 relative to
  // the total target. We penalize when P95 exceeds 80% of the target.
  const double threshold = target_ms * 0.80;
  if (p95_ms > threshold) {
    const double excess = p95_ms - threshold;
    const double penalty = std::min(50.0, excess * penalty_factor);
    sub.score -= penalty;
    if (p95_ms > target_ms) {
      sub.deductions.push_back("P95 exceeds target by " + FormatMs(p95_ms - target_ms));
    } else {
      sub.deductions.push_back("P95 is within 20% of the target; remaining headroom is " +
                               FormatMs(target_ms - p95_ms));
    }
  }

  // Additional heavy penalty if P95 exceeds the full target
  if (p95_ms > target_ms) {
    const double excess_ratio = (p95_ms - target_ms) / target_ms;
    const double penalty = std::min(30.0, excess_ratio * 40.0);
    sub.score -= penalty;
  }

  sub.score = std::max(0.0, sub.score);
  sub.grade = GradeFromScore(sub.score);
  return sub;
}

}  // namespace

FPerformanceScore ComputePerformanceScore(const FTraceStatistics& stats,
                                          const FScoreConfig& config) {
  FPerformanceScore result;

  const bool config_valid =
      std::isfinite(config.target_frame_ms) && config.target_frame_ms > 0.0 &&
      std::isfinite(config.hitch_threshold_ms) && config.hitch_threshold_ms > 0.0 &&
      std::isfinite(config.max_acceptable_hitch_pct) && config.max_acceptable_hitch_pct >= 0.0 &&
      std::isfinite(config.p95_penalty_factor) && config.p95_penalty_factor >= 0.0;
  if (stats.total_frames == 0 || !config_valid) {
    return result;
  }

  result.has_data = true;

  double overall = 100.0;

  // 1. P95 frame time penalty
  if (stats.p95_ms > config.target_frame_ms) {
    const double excess = stats.p95_ms - config.target_frame_ms;
    const double penalty = std::min(30.0, excess * config.p95_penalty_factor);
    overall -= penalty;
  }

  // 2. Hitch percentage penalty
  if (stats.hitch_percentage > config.max_acceptable_hitch_pct) {
    const double excess = stats.hitch_percentage - config.max_acceptable_hitch_pct;
    const double penalty = std::min(25.0, excess * 2.5);
    overall -= penalty;
  }

  // 3. P99 instability penalty (P99-P95 spread indicates tail instability)
  const double tail_spread = stats.p99_ms - stats.p95_ms;
  if (tail_spread > config.target_frame_ms * 0.5) {
    const double penalty = std::min(15.0, (tail_spread / config.target_frame_ms) * 10.0);
    overall -= penalty;
  }

  overall = std::max(0.0, overall);

  // Subsystem scoring
  result.subsystems.push_back(ScoreSubsystem("Game Thread", stats.game_thread_p95_ms,
                                             config.target_frame_ms, config.p95_penalty_factor));
  result.subsystems.push_back(ScoreSubsystem("Render Thread", stats.render_thread_p95_ms,
                                             config.target_frame_ms, config.p95_penalty_factor));
  result.subsystems.push_back(
      ScoreSubsystem("GPU", stats.gpu_p95_ms, config.target_frame_ms, config.p95_penalty_factor));

  // Only include domain subsystems if they have data
  if (stats.physics_p95_ms > 0.001) {
    result.subsystems.push_back(ScoreSubsystem(
        "Physics", stats.physics_p95_ms, config.target_frame_ms * 0.3, config.p95_penalty_factor));
  }
  if (stats.ai_p95_ms > 0.001) {
    result.subsystems.push_back(ScoreSubsystem("AI", stats.ai_p95_ms, config.target_frame_ms * 0.2,
                                               config.p95_penalty_factor));
  }
  if (stats.network_p95_ms > 0.001) {
    result.subsystems.push_back(ScoreSubsystem(
        "Network", stats.network_p95_ms, config.target_frame_ms * 0.15, config.p95_penalty_factor));
  }
  if (stats.loading_p95_ms > 0.001) {
    result.subsystems.push_back(ScoreSubsystem(
        "Loading", stats.loading_p95_ms, config.target_frame_ms * 0.2, config.p95_penalty_factor));
  }

  // Overall is the floor of the aggregate and the worst subsystem score
  double worst_subsystem = overall;
  bool has_subsystem_data = false;
  for (const auto& sub : result.subsystems) {
    if (sub.has_data && (!has_subsystem_data || sub.score < worst_subsystem)) {
      worst_subsystem = sub.score;
      has_subsystem_data = true;
    }
  }

  // Weighted: 70% aggregate, 30% worst subsystem
  result.overall = std::max(0.0, overall * 0.7 + worst_subsystem * 0.3);
  result.overall_grade = GradeFromScore(result.overall);

  return result;
}

}  // namespace riva

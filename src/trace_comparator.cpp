#include "riva/trace_comparator.hpp"

#include <cmath>
#include <set>
#include <string>
#include <utility>

namespace riva {
namespace {

FMetricDelta MakeDelta(const std::string& name, double baseline, double new_val, double threshold_percent = 5.0) {
  FMetricDelta delta;
  delta.metric_name = name;
  delta.baseline_value = baseline;
  delta.new_value = new_val;
  delta.delta = new_val - baseline;
  delta.delta_percent = (baseline > 0.001)
                            ? (delta.delta / baseline) * 100.0
                            : 0.0;
  delta.bRegressed = delta.delta_percent > threshold_percent;
  return delta;
}

}  // namespace

ComparisonResult DefaultTraceComparator::Compare(
    const NormalizedTrace& baseline_trace,
    const AnalysisResult& baseline,
    const NormalizedTrace& new_trace_data,
    const AnalysisResult& new_result) const {
  ComparisonResult result;

  // --- Qualitative comparison (existing: finding ID set diff) ---
  std::set<std::string> baseline_ids;
  for (const auto& rf : baseline.findings) {
    if (rf.role == FindingRole::kPrimary) {
      baseline_ids.insert(rf.finding.id);
    }
  }

  std::set<std::string> new_ids;
  for (const auto& rf : new_result.findings) {
    if (rf.role == FindingRole::kPrimary) {
      new_ids.insert(rf.finding.id);
    }
  }

  // Populate regressions (in new, not in baseline)
  for (const auto& rf : new_result.findings) {
    if (rf.role == FindingRole::kPrimary) {
      if (baseline_ids.find(rf.finding.id) == baseline_ids.end()) {
        result.regressions.push_back(rf);
      } else {
        result.unchanged.push_back(rf);
      }
    }
  }

  // Populate improvements (in baseline, not in new)
  for (const auto& rf : baseline.findings) {
    if (rf.role == FindingRole::kPrimary) {
      if (new_ids.find(rf.finding.id) == new_ids.end()) {
        result.improvements.push_back(rf);
      }
    }
  }

  // --- Quantitative comparison (new: metric-level deltas) ---
  result.statistics.baseline = ComputeTraceStatistics(baseline_trace);
  result.statistics.new_trace = ComputeTraceStatistics(new_trace_data);

  const auto& b = result.statistics.baseline;
  const auto& n = result.statistics.new_trace;

  result.statistics.metric_deltas.push_back(MakeDelta("P50 Frame Time", b.p50_ms, n.p50_ms));
  result.statistics.metric_deltas.push_back(MakeDelta("P90 Frame Time", b.p90_ms, n.p90_ms));
  result.statistics.metric_deltas.push_back(MakeDelta("P95 Frame Time", b.p95_ms, n.p95_ms));
  result.statistics.metric_deltas.push_back(MakeDelta("P99 Frame Time", b.p99_ms, n.p99_ms));
  result.statistics.metric_deltas.push_back(MakeDelta("Hitch %", b.hitch_percentage, n.hitch_percentage, 1.0));
  result.statistics.metric_deltas.push_back(MakeDelta("Game Thread P95", b.game_thread_p95_ms, n.game_thread_p95_ms));
  result.statistics.metric_deltas.push_back(MakeDelta("Render Thread P95", b.render_thread_p95_ms, n.render_thread_p95_ms));
  result.statistics.metric_deltas.push_back(MakeDelta("GPU P95", b.gpu_p95_ms, n.gpu_p95_ms));

  // Only include domain metrics if either trace has non-zero values
  if (b.physics_p95_ms > 0.001 || n.physics_p95_ms > 0.001) {
    result.statistics.metric_deltas.push_back(MakeDelta("Physics P95", b.physics_p95_ms, n.physics_p95_ms));
  }
  if (b.ai_p95_ms > 0.001 || n.ai_p95_ms > 0.001) {
    result.statistics.metric_deltas.push_back(MakeDelta("AI P95", b.ai_p95_ms, n.ai_p95_ms));
  }
  if (b.network_p95_ms > 0.001 || n.network_p95_ms > 0.001) {
    result.statistics.metric_deltas.push_back(MakeDelta("Network P95", b.network_p95_ms, n.network_p95_ms));
  }
  if (b.loading_p95_ms > 0.001 || n.loading_p95_ms > 0.001) {
    result.statistics.metric_deltas.push_back(MakeDelta("Loading P95", b.loading_p95_ms, n.loading_p95_ms));
  }

  // Overall regression flag: any metric regressed
  for (const auto& md : result.statistics.metric_deltas) {
    if (md.bRegressed) {
      result.statistics.bOverallRegressed = true;
      break;
    }
  }

  return result;
}

}  // namespace riva

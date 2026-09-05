#pragma once

#include <string>
#include <vector>

#include "riva/analysis_result.hpp"
#include "riva/export.hpp"
#include "riva/normalized_trace.hpp"
#include "riva/trace_statistics.hpp"

namespace riva {

// A single metric delta between baseline and new trace.
struct FMetricDelta {
  std::string metric_name;
  double baseline_value{0.0};
  double new_value{0.0};
  double delta{0.0};
  double delta_percent{0.0};
  bool bPercentageDefined{true};
  bool bRegressed{false};
};

// Full statistical comparison between two traces.
struct FStatisticalComparison {
  FTraceStatistics baseline;
  FTraceStatistics new_trace;
  std::vector<FMetricDelta> metric_deltas;
  bool bOverallRegressed{false};
};

struct ComparisonResult {
  std::vector<ResolvedFinding> regressions;   // In new_trace but not in baseline
  std::vector<ResolvedFinding> improvements;  // In baseline but not in new_trace
  std::vector<ResolvedFinding> unchanged;     // In both
  FStatisticalComparison statistics;
};

class ITraceComparator {
 public:
  virtual ~ITraceComparator() = default;

  [[nodiscard]] virtual ComparisonResult Compare(const NormalizedTrace& baseline_trace,
                                                 const AnalysisResult& baseline,
                                                 const NormalizedTrace& new_trace_data,
                                                 const AnalysisResult& new_result) const = 0;
};

class RIVACORE_API DefaultTraceComparator final : public ITraceComparator {
 public:
  [[nodiscard]] ComparisonResult Compare(const NormalizedTrace& baseline_trace,
                                         const AnalysisResult& baseline,
                                         const NormalizedTrace& new_trace_data,
                                         const AnalysisResult& new_result) const override;
};

}  // namespace riva

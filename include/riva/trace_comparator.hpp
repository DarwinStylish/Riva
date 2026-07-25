#pragma once

#include <vector>
#include "riva/analysis_result.hpp"

namespace riva {

struct ComparisonResult {
  std::vector<ResolvedFinding> regressions; // In new_trace but not in baseline
  std::vector<ResolvedFinding> improvements; // In baseline but not in new_trace
  std::vector<ResolvedFinding> unchanged;    // In both
};

class ITraceComparator {
 public:
  virtual ~ITraceComparator() = default;

  [[nodiscard]] virtual ComparisonResult Compare(const AnalysisResult& baseline, const AnalysisResult& new_trace) const = 0;
};

class DefaultTraceComparator final : public ITraceComparator {
 public:
  [[nodiscard]] ComparisonResult Compare(const AnalysisResult& baseline, const AnalysisResult& new_trace) const override;
};

}  // namespace riva

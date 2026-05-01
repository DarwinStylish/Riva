#pragma once

#include <vector>

#include "riva/analysis_result.hpp"
#include "riva/finding.hpp"

namespace riva {

struct ConfidenceResolverConfig {
  double minimum_confidence{0.25};
  double same_frame_conflict_window_us{2500.0};
};

class ConfidenceResolver {
 public:
  explicit ConfidenceResolver(ConfidenceResolverConfig config = {});

  [[nodiscard]] AnalysisResult Resolve(std::vector<Finding> findings) const;

 private:
  [[nodiscard]] double Calibrate(const Finding& finding) const;
  [[nodiscard]] bool ConflictsWithPrimary(const Finding& candidate, const Finding& primary) const;

  ConfidenceResolverConfig config_;
};

}  // namespace riva

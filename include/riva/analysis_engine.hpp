#pragma once

#include <memory>
#include <vector>

#include "riva/analysis_result.hpp"
#include "riva/confidence_resolver.hpp"
#include "riva/normalized_trace.hpp"
#include "riva/signature.hpp"
#include "riva/spike_detector.hpp"

namespace riva {

struct AnalysisConfig {
  SpikeDetectionConfig spike_detection;
  ConfidenceResolverConfig confidence_resolution;
};

class AnalysisEngine {
 public:
  explicit AnalysisEngine(std::vector<std::unique_ptr<ISignature>> signatures,
                          AnalysisConfig config = {});

  [[nodiscard]] AnalysisResult Analyze(const NormalizedTrace& trace) const;

 private:
  std::vector<std::unique_ptr<ISignature>> signatures_;
  AnalysisConfig config_;
};

}  // namespace riva

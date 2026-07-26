#pragma once

#include <memory>
#include <vector>

#include "riva/analysis_result.hpp"
#include "riva/confidence_resolver.hpp"
#include "riva/normalized_trace.hpp"
#include "riva/signature.hpp"
#include "riva/spike_detector.hpp"
#include "riva/causal_chain_resolver.hpp"
#include "riva/correlation_resolver.hpp"
#include "riva/budget.hpp"
#include <optional>

namespace riva {

struct AnalysisConfig {
  SpikeDetectionConfig spike_detection;
  CausalChainResolverConfig causal_chain_resolution;
  ConfidenceResolverConfig confidence_resolution;
  CorrelationResolverConfig correlation_resolution;
  std::optional<BudgetConfig> budget;
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

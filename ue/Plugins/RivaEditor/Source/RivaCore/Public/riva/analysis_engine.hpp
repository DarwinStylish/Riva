#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "riva/analysis_result.hpp"
#include "riva/budget.hpp"
#include "riva/causal_chain_resolver.hpp"
#include "riva/confidence_resolver.hpp"
#include "riva/correlation_resolver.hpp"
#include "riva/export.hpp"
#include "riva/normalized_trace.hpp"
#include "riva/performance_score.hpp"
#include "riva/signature.hpp"
#include "riva/spike_detector.hpp"

namespace riva {

struct AnalysisConfig {
  SpikeDetectionConfig spike_detection;
  CausalChainResolverConfig causal_chain_resolution;
  ConfidenceResolverConfig confidence_resolution;
  CorrelationResolverConfig correlation_resolution;
  FScoreConfig performance_score;
  std::optional<BudgetConfig> budget;
};

class RIVACORE_API AnalysisEngine {
 public:
  explicit AnalysisEngine(std::vector<std::unique_ptr<ISignature>> signatures,
                          const AnalysisConfig& config = {});

  [[nodiscard]] AnalysisResult Analyze(const NormalizedTrace& trace) const;

 private:
  std::vector<std::unique_ptr<ISignature>> signatures_;
  AnalysisConfig config_;
};

}  // namespace riva

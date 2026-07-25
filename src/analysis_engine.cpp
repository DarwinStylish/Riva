#include "riva/analysis_engine.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace riva {

AnalysisEngine::AnalysisEngine(std::vector<std::unique_ptr<ISignature>> signatures,
                               AnalysisConfig config)
    : signatures_(std::move(signatures)), config_(config) {}

AnalysisResult AnalysisEngine::Analyze(const NormalizedTrace& trace) const {
  RollingMedianSpikeDetector detector(config_.spike_detection);
  const auto spikes = detector.Detect(trace);

  std::vector<Finding> raw_findings;

  for (const auto& signature : signatures_) {
    if (!signature) {
      continue;
    }

    auto signature_findings = signature->Analyze(trace, spikes);
    raw_findings.insert(
        raw_findings.end(),
        std::make_move_iterator(signature_findings.begin()),
        std::make_move_iterator(signature_findings.end()));
  }

  DefaultCausalChainResolver causal_resolver(config_.causal_chain_resolution);
  raw_findings = causal_resolver.Resolve(std::move(raw_findings));

  ConfidenceResolver resolver(config_.confidence_resolution);
  auto result = resolver.Resolve(std::move(raw_findings));

  DefaultCorrelationResolver correlator(config_.correlation_resolution);
  result.findings = correlator.Resolve(result.findings);

  result.total_frames_analyzed = trace.frame_count();
  result.hitch_count = spikes.size();
  result.source_name = trace.source_name();
  return result;
}

}  // namespace riva

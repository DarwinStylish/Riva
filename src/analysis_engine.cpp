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

  if (config_.budget.has_value()) {
    const auto& budget = *config_.budget;
    bool game_thread_breached = false;
    bool render_thread_breached = false;
    bool rhi_thread_breached = false;
    bool gpu_breached = false;
    bool duration_breached = false;

    for (const auto& frame : trace.frames()) {
      if (budget.game_thread_ms_max.has_value() && frame.game_thread_ms > *budget.game_thread_ms_max) {
        game_thread_breached = true;
      }
      if (budget.render_thread_ms_max.has_value() && frame.render_thread_ms > *budget.render_thread_ms_max) {
        render_thread_breached = true;
      }
      if (budget.rhi_thread_ms_max.has_value() && frame.rhi_thread_ms > *budget.rhi_thread_ms_max) {
        rhi_thread_breached = true;
      }
      if (budget.gpu_ms_max.has_value() && frame.gpu_ms > *budget.gpu_ms_max) {
        gpu_breached = true;
      }
      if (budget.duration_ms_max.has_value() && frame.duration_ms > *budget.duration_ms_max) {
        duration_breached = true;
      }
    }

    if (game_thread_breached) result.budget_status.breached_metrics.push_back("game_thread_ms");
    if (render_thread_breached) result.budget_status.breached_metrics.push_back("render_thread_ms");
    if (rhi_thread_breached) result.budget_status.breached_metrics.push_back("rhi_thread_ms");
    if (gpu_breached) result.budget_status.breached_metrics.push_back("gpu_ms");
    if (duration_breached) result.budget_status.breached_metrics.push_back("duration_ms");

    result.budget_status.breached = !result.budget_status.breached_metrics.empty();
  }

  return result;
}

}  // namespace riva

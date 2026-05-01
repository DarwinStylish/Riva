#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "riva/analysis_engine.hpp"
#include "riva/builtin_signatures.hpp"
#include "riva/confidence_resolver.hpp"
#include "riva/normalized_trace.hpp"

namespace {

void Expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << "\n";
    std::exit(1);
  }
}

riva::Finding MakeFinding(std::string id, double confidence, riva::Severity severity) {
  riva::Finding finding;
  finding.id = std::move(id);
  finding.title = "Test finding";
  finding.severity = severity;
  finding.confidence = confidence;
  finding.frame_index = 7;
  finding.time_window_start_us = 100000;
  finding.time_window_end_us = 120000;
  finding.evidence.push_back(riva::Evidence{"frame_ms", "50.00 ms"});
  finding.suggested_next_steps.push_back("Inspect the relevant lane in Unreal Insights.");
  finding.how_to_confirm.push_back("Confirm the event overlaps the spike window.");
  return finding;
}

riva::TraceEvent MakeEvent(std::string name, std::uint64_t start_us, std::uint64_t duration_us) {
  riva::TraceEvent event;
  event.name = std::move(name);
  event.category = "Test";
  event.thread_name = "GameThread";
  event.start_time_us = start_us;
  event.duration_us = duration_us;
  return event;
}

riva::NormalizedTrace MakeTraceWithMultipleSignals() {
  riva::NormalizedTrace trace("analysis-engine-test");

  for (std::size_t i = 0; i < 6; ++i) {
    riva::Frame frame;
    frame.index = i;
    frame.start_time_us = static_cast<std::uint64_t>(i * 16000);
    frame.duration_ms = i == 5 ? 50.0 : 16.0;
    frame.game_thread_ms = i == 5 ? 44.0 : 10.0;
    frame.render_thread_ms = i == 5 ? 38.0 : 9.0;
    frame.gpu_ms = i == 5 ? 35.0 : 12.0;

    if (i == 5) {
      frame.events.push_back(MakeEvent("ShaderCompileWorker blocked frame", frame.start_time_us, 4000));
      frame.events.push_back(MakeEvent("GarbageCollect mark sweep", frame.start_time_us + 1000, 5000));
    }

    auto status = trace.AddFrame(std::move(frame));
    Expect(status.ok(), "frame should be accepted");
  }

  return trace;
}

void TestResolverSelectsPrimaryAndSecondary() {
  std::vector<riva::Finding> findings;
  findings.push_back(MakeFinding("LOW", 0.45, riva::Severity::kWarning));
  findings.push_back(MakeFinding("HIGH", 0.85, riva::Severity::kCritical));

  riva::ConfidenceResolver resolver;
  const auto result = resolver.Resolve(std::move(findings));

  Expect(result.findings.size() == 2, "resolver should keep both findings");
  Expect(result.findings[0].finding.id == "HIGH", "highest confidence finding should sort first");
  Expect(result.findings[0].role == riva::FindingRole::kPrimary, "first finding should be primary");
  Expect(result.findings[1].role == riva::FindingRole::kSecondary, "conflicting finding should be secondary");
}

void TestResolverDropsWeakFindings() {
  std::vector<riva::Finding> findings;
  auto weak = MakeFinding("WEAK", 0.20, riva::Severity::kInfo);
  weak.evidence.clear();
  findings.push_back(std::move(weak));

  riva::ConfidenceResolver resolver;
  const auto result = resolver.Resolve(std::move(findings));

  Expect(result.findings.empty(), "weak unsupported findings should be dropped");
}

void TestAnalysisEngineRunsSignaturesAndResolution() {
  auto trace = MakeTraceWithMultipleSignals();

  riva::AnalysisEngine engine(riva::CreateBuiltinSignatures());
  const auto result = engine.Analyze(trace);

  Expect(!result.findings.empty(), "analysis engine should produce resolved findings");

  bool saw_primary = false;
  bool saw_confirmation = false;

  for (const auto& resolved : result.findings) {
    if (resolved.role == riva::FindingRole::kPrimary) {
      saw_primary = true;
    }

    if (!resolved.finding.how_to_confirm.empty()) {
      saw_confirmation = true;
    }

    Expect(resolved.finding.confidence >= 0.25, "resolved confidence should pass minimum threshold");
    Expect(resolved.finding.confidence <= 0.95, "resolved confidence should be capped conservatively");
  }

  Expect(saw_primary, "analysis result should include a primary finding");
  Expect(saw_confirmation, "resolved findings must preserve how-to-confirm guidance");
}

}  // namespace

int main() {
  TestResolverSelectsPrimaryAndSecondary();
  TestResolverDropsWeakFindings();
  TestAnalysisEngineRunsSignaturesAndResolution();
  return 0;
}

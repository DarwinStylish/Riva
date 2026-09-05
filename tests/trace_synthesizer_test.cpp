#include "riva/trace_synthesizer.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

#include "riva/analysis_engine.hpp"
#include "riva/builtin_signatures.hpp"

namespace {

void Expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << "\n";
    std::exit(1);
  }
}

void TestGeneratesCorrectFrameCount() {
  riva::FTraceSynthesizerConfig config;
  config.source_name = "frame-count-test";
  config.frame_count = 60;

  auto result = riva::FTraceSynthesizer::Generate(config);
  Expect(result.status.ok(), "valid frame-count configuration must succeed");
  Expect(result.trace.frame_count() == 60, "should generate exactly 60 frames");
  Expect(result.trace.source_name() == "frame-count-test", "source name must match");
}

void TestBaselineFramesAreStable() {
  riva::FTraceSynthesizerConfig config;
  config.source_name = "baseline-test";
  config.frame_count = 50;
  config.baseline_frame_ms = 16.0;
  config.baseline_jitter_ms = 0.5;

  auto result = riva::FTraceSynthesizer::Generate(config);
  Expect(result.status.ok(), "valid baseline configuration must succeed");

  for (const auto& frame : result.trace.frames()) {
    Expect(frame.duration_ms > 14.0 && frame.duration_ms < 18.0,
           "baseline frames must be within jitter range");
  }
  Expect(result.ground_truth.empty(), "no injections means no ground truth");
}

void TestPathologyInjection() {
  riva::FTraceSynthesizerConfig config;
  config.source_name = "injection-test";
  config.frame_count = 30;
  config.baseline_frame_ms = 16.0;

  riva::FPathologyInjection injection;
  injection.type = riva::EPathologyType::kShaderCompile;
  injection.frame_index = 15;
  injection.spike_ms = 55.0;
  config.injections.push_back(injection);

  auto result = riva::FTraceSynthesizer::Generate(config);
  Expect(result.status.ok(), "valid injection must succeed");

  // Verify spike frame
  Expect(result.trace.frames()[15].duration_ms == 55.0, "spike frame must have injected duration");
  Expect(!result.trace.frames()[15].events.empty(), "spike frame must have marker events");
  Expect(result.trace.frames()[15].events[0].name == "ShaderCompileWorker blocked frame",
         "spike frame must have correct marker name");
  Expect(result.trace.frames()[15].events[0].thread_name == "GameThread",
         "shader compile event must be on GameThread");

  // Ground truth
  Expect(result.ground_truth.size() == 1, "one ground truth entry");
  Expect(result.ground_truth[0].type == riva::EPathologyType::kShaderCompile,
         "ground truth type must match");
  Expect(result.ground_truth[0].frame_index == 15, "ground truth frame index must match");
}

void TestMultiplePathologies() {
  riva::FTraceSynthesizerConfig config;
  config.source_name = "multi-injection";
  config.frame_count = 100;
  config.baseline_frame_ms = 16.0;

  config.injections.push_back({riva::EPathologyType::kShaderCompile, 20, 48.0, ""});
  config.injections.push_back({riva::EPathologyType::kGarbageCollection, 50, 52.0, ""});
  config.injections.push_back({riva::EPathologyType::kStreamingIo, 80, 45.0, ""});

  auto result = riva::FTraceSynthesizer::Generate(config);
  Expect(result.status.ok(), "valid multiple injections must succeed");

  Expect(result.ground_truth.size() == 3, "three ground truth entries");
  Expect(result.trace.frames()[20].duration_ms == 48.0, "first spike must match");
  Expect(result.trace.frames()[50].duration_ms == 52.0, "second spike must match");
  Expect(result.trace.frames()[80].duration_ms == 45.0, "third spike must match");

  // Verify different marker names
  Expect(result.trace.frames()[20].events[0].name == "ShaderCompileWorker blocked frame",
         "shader marker");
  Expect(result.trace.frames()[50].events[0].name == "GarbageCollect mark sweep", "gc marker");
  Expect(result.trace.frames()[80].events[0].name == "Async Loading streaming IO request",
         "io marker");
}

void TestDeterminism() {
  riva::FTraceSynthesizerConfig config;
  config.source_name = "determinism-test";
  config.frame_count = 100;
  config.baseline_frame_ms = 16.0;
  config.baseline_jitter_ms = 1.0;
  config.injections.push_back({riva::EPathologyType::kShaderCompile, 42, 50.0, ""});

  auto result1 = riva::FTraceSynthesizer::Generate(config);
  auto result2 = riva::FTraceSynthesizer::Generate(config);
  Expect(result1.status.ok() && result2.status.ok(),
         "valid deterministic configurations must succeed");

  Expect(result1.trace.frame_count() == result2.trace.frame_count(),
         "determinism: frame counts must match");

  for (std::size_t i = 0; i < result1.trace.frame_count(); ++i) {
    Expect(result1.trace.frames()[i].duration_ms == result2.trace.frames()[i].duration_ms,
           "determinism: all frame durations must be identical");
  }
}

void TestAnalysisDetectsInjectedPathology() {
  riva::FTraceSynthesizerConfig config;
  config.source_name = "analysis-detection";
  config.frame_count = 120;
  config.baseline_frame_ms = 16.0;
  config.baseline_jitter_ms = 0.3;
  config.injections.push_back({riva::EPathologyType::kShaderCompile, 60, 55.0, ""});

  auto synthesized = riva::FTraceSynthesizer::Generate(config);
  Expect(synthesized.status.ok(), "valid analysis fixture must succeed");

  // Run the full analysis pipeline
  auto signatures = riva::CreateBuiltinSignatures();
  riva::AnalysisEngine engine(std::move(signatures));
  auto analysis = engine.Analyze(synthesized.trace);

  // The shader compile signature should fire
  bool found_shader = false;
  for (const auto& rf : analysis.findings) {
    if (rf.finding.id == "STUT_SHADER_COMPILE") {
      found_shader = true;
      Expect(rf.finding.frame_index == 60, "shader finding must point to the injected frame");
      break;
    }
  }
  Expect(found_shader, "analysis must detect the injected shader compile pathology");
}

void TestBuildInfoAndScenarioRoundTrip() {
  riva::FTraceSynthesizerConfig config;
  config.source_name = "metadata-test";
  config.frame_count = 10;
  config.build_info.build_id = "B-999";
  config.build_info.branch = "main";
  config.build_info.platform = "Win64";
  config.scenario_info.scenario_id = "test-scenario";
  config.scenario_info.map_name = "TestMap_P";

  auto result = riva::FTraceSynthesizer::Generate(config);
  Expect(result.status.ok(), "valid metadata fixture must succeed");

  Expect(result.trace.build_info().build_id == "B-999", "build_id must round-trip");
  Expect(result.trace.build_info().branch == "main", "branch must round-trip");
  Expect(result.trace.scenario_info().scenario_id == "test-scenario",
         "scenario_id must round-trip");
  Expect(result.trace.scenario_info().map_name == "TestMap_P", "map_name must round-trip");
}

void TestRejectsInvalidConfiguration() {
  riva::FTraceSynthesizerConfig config;
  config.baseline_jitter_ms = config.baseline_frame_ms;
  auto result = riva::FTraceSynthesizer::Generate(config);
  Expect(!result.status.ok(), "jitter that can produce non-positive frames must be rejected");
  Expect(result.trace.empty(), "invalid configuration must not return a partial trace");

  config = riva::FTraceSynthesizerConfig{};
  config.frame_count = 5;
  config.injections.push_back({riva::EPathologyType::kShaderCompile, 5, 50.0, ""});
  result = riva::FTraceSynthesizer::Generate(config);
  Expect(!result.status.ok(), "out-of-range injection must be rejected");

  config = riva::FTraceSynthesizerConfig{};
  config.frame_count = 5;
  config.injections.push_back({riva::EPathologyType::kShaderCompile, 2, 50.0, ""});
  config.injections.push_back({riva::EPathologyType::kPsoMiss, 2, 50.0, ""});
  result = riva::FTraceSynthesizer::Generate(config);
  Expect(!result.status.ok(), "duplicate injection frames must be rejected");
}

}  // namespace

int main() {
  TestGeneratesCorrectFrameCount();
  TestBaselineFramesAreStable();
  TestPathologyInjection();
  TestMultiplePathologies();
  TestDeterminism();
  TestAnalysisDetectsInjectedPathology();
  TestBuildInfoAndScenarioRoundTrip();
  TestRejectsInvalidConfiguration();
  std::cout << "All trace synthesizer tests passed successfully!\n";
  return 0;
}

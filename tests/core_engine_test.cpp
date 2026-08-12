#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "riva/finding.hpp"
#include "riva/normalized_trace.hpp"
#include "riva/build_info.hpp"
#include "riva/thread.hpp"
#include "riva/counter.hpp"
#include "riva/signature.hpp"
#include "riva/spike_detector.hpp"

namespace {

void Expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << "\n";
    std::exit(1);
  }
}

class DummySignature final : public riva::ISignature {
 public:
  [[nodiscard]] std::string id() const override {
    return "DUMMY";
  }

  [[nodiscard]] std::string name() const override {
    return "Dummy Signature";
  }

  [[nodiscard]] std::vector<riva::Finding> Analyze(
      const riva::NormalizedTrace& trace,
      const std::vector<riva::Spike>& spikes) const override {
    std::vector<riva::Finding> findings;

    for (const auto& spike : spikes) {
      const auto& frame = trace.frames()[spike.frame_index];
      findings.push_back(riva::Finding{
          "DUMMY",
          "Dummy spike finding",
          riva::Severity::kWarning,
          0.5,
          spike.frame_index,
          frame.start_time_us,
          frame.start_time_us + 1000,
          {riva::Evidence{"frame_ms", std::to_string(spike.frame_ms)}},
          {"Inspect the frame in Unreal Insights."},
          {"Open the same frame range in Unreal Insights and verify timing contributors."},
      });
    }

    return findings;
  }
};

void TestNormalizedTraceRejectsInvalidFrames() {
  riva::NormalizedTrace trace("unit-test");

  Expect(trace.source_name() == "unit-test", "source name must be preserved");
  Expect(trace.empty(), "new trace must be empty");

  auto first = trace.AddFrame(riva::Frame{0, 1000, 16.0});
  Expect(first.ok(), "first frame should be accepted");

  auto duplicate = trace.AddFrame(riva::Frame{0, 2000, 16.0});
  Expect(!duplicate.ok(), "duplicate frame index should be rejected");

  auto negative = trace.AddFrame(riva::Frame{1, 3000, -1.0});
  Expect(!negative.ok(), "negative frame duration should be rejected");

  Expect(trace.frame_count() == 1, "only one frame should remain accepted");
}

void TestMedian() {
  Expect(riva::Median({}) == 0.0, "empty median must be zero");
  Expect(riva::Median({3.0}) == 3.0, "single median must equal value");
  Expect(riva::Median({3.0, 1.0, 2.0}) == 2.0, "odd median must sort deterministically");
  Expect(riva::Median({4.0, 2.0, 8.0, 6.0}) == 5.0, "even median must average middle values");
}

void TestSpikeDetection() {
  riva::NormalizedTrace trace("spike-test");

  const std::vector<double> frame_times = {
      16.0,
      16.1,
      15.9,
      16.0,
      16.2,
      48.0,
      16.0,
  };

  for (std::size_t i = 0; i < frame_times.size(); ++i) {
    auto status = trace.AddFrame(
        riva::Frame{i, static_cast<std::uint64_t>(i * 16000), frame_times[i]});
    Expect(status.ok(), "frame should be accepted");
  }

  riva::RollingMedianSpikeDetector detector;
  const auto spikes = detector.Detect(trace);

  Expect(spikes.size() == 1, "one spike should be detected");
  Expect(spikes[0].frame_index == 5, "spike should point to frame 5");
  Expect(spikes[0].frame_ms == 48.0, "spike frame duration must be preserved");
  Expect(spikes[0].baseline_ms > 15.0 && spikes[0].baseline_ms < 17.0,
         "baseline must use rolling median");
  Expect(spikes[0].ratio >= 2.0, "spike ratio should exceed threshold");
}

void TestSignatureInterface() {
  riva::NormalizedTrace trace("signature-test");

  for (std::size_t i = 0; i < 6; ++i) {
    const double frame_ms = i == 5 ? 50.0 : 16.0;
    auto status = trace.AddFrame(
        riva::Frame{i, static_cast<std::uint64_t>(i * 16000), frame_ms});
    Expect(status.ok(), "frame should be accepted");
  }

  riva::RollingMedianSpikeDetector detector;
  const auto spikes = detector.Detect(trace);

  DummySignature signature;
  const auto findings = signature.Analyze(trace, spikes);

  Expect(signature.id() == "DUMMY", "signature id must be stable");
  Expect(findings.size() == 1, "signature should produce one finding");
  Expect(findings[0].severity == riva::Severity::kWarning, "finding severity must be preserved");
  Expect(!findings[0].how_to_confirm.empty(), "finding must include confirmation guidance");
}

void TestExpandedFrameFields() {
  riva::NormalizedTrace trace("expanded-test");

  riva::Frame frame;
  frame.index = 0;
  frame.start_time_us = 0;
  frame.duration_ms = 16.0;
  frame.game_thread_ms = 10.0;
  frame.render_thread_ms = 8.0;
  frame.rhi_thread_ms = 3.0;
  frame.gpu_ms = 11.0;
  frame.physics_ms = 2.5;
  frame.ai_ms = 1.3;
  frame.network_ms = 0.8;
  frame.loading_ms = 0.4;
  frame.memory_bytes = 1024.0 * 1024.0 * 512.0;

  riva::FTraceCounter counter;
  counter.name = "PhysicsObjects";
  counter.category = "Physics";
  counter.unit = "count";
  counter.timestamp_us = 1000;
  counter.value = 42.0;
  frame.counters.push_back(counter);

  auto status = trace.AddFrame(std::move(frame));
  Expect(status.ok(), "expanded frame should be accepted");

  const auto& stored = trace.frames()[0];
  Expect(stored.physics_ms == 2.5, "physics_ms must be preserved");
  Expect(stored.ai_ms == 1.3, "ai_ms must be preserved");
  Expect(stored.network_ms == 0.8, "network_ms must be preserved");
  Expect(stored.loading_ms == 0.4, "loading_ms must be preserved");
  Expect(stored.memory_bytes == 1024.0 * 1024.0 * 512.0, "memory_bytes must be preserved");
  Expect(stored.counters.size() == 1, "counters must be preserved");
  Expect(stored.counters[0].name == "PhysicsObjects", "counter name must be preserved");
  Expect(stored.counters[0].value == 42.0, "counter value must be preserved");
}

void TestBuildInfoAndScenarioInfo() {
  riva::NormalizedTrace trace("build-test");

  riva::FBuildInfo build;
  build.build_id = "B-100";
  build.version = "0.2.0";
  build.branch = "feat/nanite";
  build.commit = "abc123";
  build.configuration = "Development";
  build.platform = "Win64";
  build.engine_version = "5.5.1";
  build.timestamp = 1722096000;
  trace.SetBuildInfo(std::move(build));

  riva::FScenarioInfo scenario;
  scenario.scenario_id = "forest-flythrough";
  scenario.name = "Forest Flythrough";
  scenario.map_name = "ForestBenchmark_P";
  scenario.gameplay_state = "gameplay";
  scenario.player_count = 1;
  scenario.agent_count = 50;
  trace.SetScenarioInfo(std::move(scenario));

  Expect(trace.build_info().build_id == "B-100", "build_id must be preserved");
  Expect(trace.build_info().branch == "feat/nanite", "branch must be preserved");
  Expect(trace.build_info().platform == "Win64", "platform must be preserved");
  Expect(trace.build_info().engine_version == "5.5.1", "engine_version must be preserved");
  Expect(trace.build_info().timestamp == 1722096000, "timestamp must be preserved");

  Expect(trace.scenario_info().scenario_id == "forest-flythrough", "scenario_id must be preserved");
  Expect(trace.scenario_info().map_name == "ForestBenchmark_P", "map_name must be preserved");
  Expect(trace.scenario_info().player_count == 1, "player_count must be preserved");
  Expect(trace.scenario_info().agent_count == 50, "agent_count must be preserved");
}

void TestAddThreadRejectsDuplicates() {
  riva::NormalizedTrace trace("thread-test");

  riva::FTraceThread thread1;
  thread1.id = 0;
  thread1.name = "GameThread";
  thread1.type = riva::EThreadType::kGameThread;
  thread1.utilization = 0.85;

  auto status = trace.AddThread(std::move(thread1));
  Expect(status.ok(), "first thread should be accepted");
  Expect(trace.threads().size() == 1, "one thread should be stored");
  Expect(trace.threads()[0].name == "GameThread", "thread name must be preserved");
  Expect(trace.threads()[0].type == riva::EThreadType::kGameThread, "thread type must be preserved");

  riva::FTraceThread thread2;
  thread2.id = 1;
  thread2.name = "RenderThread";
  thread2.type = riva::EThreadType::kRenderThread;
  thread2.utilization = 0.72;

  status = trace.AddThread(std::move(thread2));
  Expect(status.ok(), "second thread should be accepted");
  Expect(trace.threads().size() == 2, "two threads should be stored");

  riva::FTraceThread duplicate;
  duplicate.id = 0;
  duplicate.name = "DuplicateGameThread";
  duplicate.type = riva::EThreadType::kGameThread;

  status = trace.AddThread(std::move(duplicate));
  Expect(!status.ok(), "duplicate thread id should be rejected");
  Expect(trace.threads().size() == 2, "thread count should not change after rejection");
}

void TestEvidenceClassification() {
  riva::Evidence observed{"frame_ms", "16.0 ms", riva::EEvidenceClassification::kObserved};
  riva::Evidence derived{"delta_ms", "32.0 ms", riva::EEvidenceClassification::kDerived};
  riva::Evidence correlated{"cluster", "3 spikes", riva::EEvidenceClassification::kCorrelated};
  riva::Evidence inferred{"cause", "gc stall", riva::EEvidenceClassification::kInferred};
  riva::Evidence suspected{"hypothesis", "memory pressure", riva::EEvidenceClassification::kSuspected};
  riva::Evidence recommended{"action", "precompile shaders", riva::EEvidenceClassification::kRecommended};

  Expect(observed.classification == riva::EEvidenceClassification::kObserved, "observed classification");
  Expect(derived.classification == riva::EEvidenceClassification::kDerived, "derived classification");
  Expect(correlated.classification == riva::EEvidenceClassification::kCorrelated, "correlated classification");
  Expect(inferred.classification == riva::EEvidenceClassification::kInferred, "inferred classification");
  Expect(suspected.classification == riva::EEvidenceClassification::kSuspected, "suspected classification");
  Expect(recommended.classification == riva::EEvidenceClassification::kRecommended, "recommended classification");
}

}  // namespace

int main() {
  TestNormalizedTraceRejectsInvalidFrames();
  TestMedian();
  TestSpikeDetection();
  TestSignatureInterface();
  TestExpandedFrameFields();
  TestBuildInfoAndScenarioInfo();
  TestAddThreadRejectsDuplicates();
  TestEvidenceClassification();
  return 0;
}

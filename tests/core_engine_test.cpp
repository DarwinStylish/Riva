#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "riva/build_info.hpp"
#include "riva/counter.hpp"
#include "riva/finding.hpp"
#include "riva/normalized_trace.hpp"
#include "riva/signature.hpp"
#include "riva/signature_utils.hpp"
#include "riva/spike_detector.hpp"
#include "riva/thread.hpp"

namespace {

void Expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << "\n";
    std::exit(1);
  }
}

riva::Frame MakeFrame(std::size_t index, std::uint64_t start_time_us, double duration_ms) {
  riva::Frame frame;
  frame.index = index;
  frame.start_time_us = start_time_us;
  frame.duration_ms = duration_ms;
  return frame;
}

class TestSignature final : public riva::ISignature {
 public:
  [[nodiscard]] std::string id() const override { return "TEST"; }

  [[nodiscard]] std::string name() const override { return "Test Signature"; }

  [[nodiscard]] std::vector<riva::Finding> Analyze(
      const riva::NormalizedTrace& trace, const std::vector<riva::Spike>& spikes) const override {
    std::vector<riva::Finding> findings;

    for (const auto& spike : spikes) {
      const riva::SpikeContext context = riva::BuildSpikeContext(trace, spike);
      if (context.frame == nullptr) {
        continue;
      }
      const auto& frame = *context.frame;
      riva::Finding finding;
      finding.id = "TEST";
      finding.title = "Test spike finding";
      finding.severity = riva::Severity::kWarning;
      finding.confidence = 0.5;
      finding.frame_index = spike.frame_index;
      finding.time_window_start_us = frame.start_time_us;
      finding.time_window_end_us = frame.start_time_us + 1000;
      finding.evidence.push_back(riva::Evidence{"frame_ms", std::to_string(spike.frame_ms)});
      finding.suggested_next_steps.push_back("Inspect the frame in Unreal Insights.");
      finding.how_to_confirm.push_back(
          "Open the same frame range in Unreal Insights and verify timing contributors.");
      findings.push_back(std::move(finding));
    }

    return findings;
  }
};

void TestNormalizedTraceRejectsInvalidFrames() {
  riva::NormalizedTrace trace("unit-test");

  Expect(trace.source_name() == "unit-test", "source name must be preserved");
  Expect(trace.empty(), "new trace must be empty");

  auto first = trace.AddFrame(MakeFrame(0, 1000, 16.0));
  Expect(first.ok(), "first frame should be accepted");

  auto duplicate = trace.AddFrame(MakeFrame(0, 2000, 16.0));
  Expect(!duplicate.ok(), "duplicate frame index should be rejected");

  auto negative = trace.AddFrame(MakeFrame(1, 3000, -1.0));
  Expect(!negative.ok(), "negative frame duration should be rejected");

  riva::Frame non_finite;
  non_finite.index = 1;
  non_finite.start_time_us = 3000;
  non_finite.duration_ms = std::numeric_limits<double>::quiet_NaN();
  Expect(!trace.AddFrame(std::move(non_finite)).ok(),
         "non-finite frame duration should be rejected");

  riva::Frame overflowing;
  overflowing.index = 1;
  overflowing.start_time_us = std::numeric_limits<std::uint64_t>::max() - 500;
  overflowing.duration_ms = 1.0;
  Expect(!trace.AddFrame(std::move(overflowing)).ok(),
         "overflowing frame time range should be rejected");

  Expect(trace.frame_count() == 1, "only one frame should remain accepted");

  riva::NormalizedTrace ordered("ordered");
  riva::Frame first_ordered;
  first_ordered.index = 1;
  first_ordered.start_time_us = 1000;
  first_ordered.duration_ms = 1.0;
  Expect(ordered.AddFrame(std::move(first_ordered)).ok(), "first ordered frame must be accepted");

  riva::Frame backwards;
  backwards.index = 2;
  backwards.start_time_us = 999;
  backwards.duration_ms = 1.0;
  Expect(!ordered.AddFrame(std::move(backwards)).ok(),
         "backward frame timestamps must be rejected");

  riva::NormalizedTrace counters("counters");
  riva::Frame bad_counter;
  bad_counter.index = 1;
  bad_counter.duration_ms = 1.0;
  bad_counter.counters.push_back(
      riva::FTraceCounter{"counter", "", "", 0, std::numeric_limits<double>::infinity()});
  Expect(!counters.AddFrame(std::move(bad_counter)).ok(),
         "non-finite counter values must be rejected");
}

void TestSparseFrameIndicesResolveById() {
  riva::NormalizedTrace trace("sparse-index-test");
  const std::vector<std::size_t> frame_ids = {10, 20, 30, 40, 50, 60};

  for (std::size_t position = 0; position < frame_ids.size(); ++position) {
    const double frame_ms = position + 1 == frame_ids.size() ? 48.0 : 16.0;
    const auto status = trace.AddFrame(
        MakeFrame(frame_ids[position], static_cast<std::uint64_t>(position * 16000), frame_ms));
    Expect(status.ok(), "sparse but increasing frame IDs should be accepted");
  }

  const auto spikes = riva::RollingMedianSpikeDetector{}.Detect(trace);
  Expect(spikes.size() == 1, "sparse-ID trace should contain one spike");
  Expect(spikes[0].frame_index == 60, "spike should preserve the external frame ID");

  const riva::SpikeContext context = riva::BuildSpikeContext(trace, spikes[0]);
  Expect(context.frame != nullptr, "spike context should resolve a sparse frame ID");
  Expect(context.frame->index == 60, "spike context should resolve the correct frame");
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
      16.0, 16.1, 15.9, 16.0, 16.2, 48.0, 16.0,
  };

  for (std::size_t i = 0; i < frame_times.size(); ++i) {
    auto status =
        trace.AddFrame(MakeFrame(i, static_cast<std::uint64_t>(i * 16000), frame_times[i]));
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
    auto status = trace.AddFrame(MakeFrame(i, static_cast<std::uint64_t>(i * 16000), frame_ms));
    Expect(status.ok(), "frame should be accepted");
  }

  riva::RollingMedianSpikeDetector detector;
  const auto spikes = detector.Detect(trace);

  TestSignature signature;
  const auto findings = signature.Analyze(trace, spikes);

  Expect(signature.id() == "TEST", "signature id must be stable");
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
  Expect(trace.threads()[0].type == riva::EThreadType::kGameThread,
         "thread type must be preserved");

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
  riva::Evidence suspected{"hypothesis", "memory pressure",
                           riva::EEvidenceClassification::kSuspected};
  riva::Evidence recommended{"action", "precompile shaders",
                             riva::EEvidenceClassification::kRecommended};

  Expect(observed.classification == riva::EEvidenceClassification::kObserved,
         "observed classification");
  Expect(derived.classification == riva::EEvidenceClassification::kDerived,
         "derived classification");
  Expect(correlated.classification == riva::EEvidenceClassification::kCorrelated,
         "correlated classification");
  Expect(inferred.classification == riva::EEvidenceClassification::kInferred,
         "inferred classification");
  Expect(suspected.classification == riva::EEvidenceClassification::kSuspected,
         "suspected classification");
  Expect(recommended.classification == riva::EEvidenceClassification::kRecommended,
         "recommended classification");
}

}  // namespace

int main() {
  TestNormalizedTraceRejectsInvalidFrames();
  TestMedian();
  TestSpikeDetection();
  TestSparseFrameIndicesResolveById();
  TestSignatureInterface();
  TestExpandedFrameFields();
  TestBuildInfoAndScenarioInfo();
  TestAddThreadRejectsDuplicates();
  TestEvidenceClassification();
  return 0;
}

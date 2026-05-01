#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "riva/finding.hpp"
#include "riva/normalized_trace.hpp"
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

}  // namespace

int main() {
  TestNormalizedTraceRejectsInvalidFrames();
  TestMedian();
  TestSpikeDetection();
  TestSignatureInterface();
  return 0;
}

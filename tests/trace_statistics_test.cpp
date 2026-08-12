#include <cmath>
#include <cstdlib>
#include <iostream>

#include "riva/normalized_trace.hpp"
#include "riva/trace_statistics.hpp"

namespace {

void Expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << "\n";
    std::exit(1);
  }
}

bool ApproxEqual(double a, double b, double epsilon = 0.01) {
  return std::fabs(a - b) < epsilon;
}

riva::NormalizedTrace MakeUniformTrace(std::size_t count, double duration_ms) {
  riva::NormalizedTrace trace("stats-uniform");
  for (std::size_t i = 0; i < count; ++i) {
    riva::Frame frame;
    frame.index = i;
    frame.start_time_us = static_cast<std::uint64_t>(i * 16000);
    frame.duration_ms = duration_ms;
    frame.game_thread_ms = duration_ms * 0.625;
    frame.render_thread_ms = duration_ms * 0.5;
    frame.gpu_ms = duration_ms * 0.7;
    (void)trace.AddFrame(std::move(frame));
  }
  return trace;
}

void TestEmptyTrace() {
  riva::NormalizedTrace trace("empty");
  auto stats = riva::ComputeTraceStatistics(trace);

  Expect(stats.total_frames == 0, "empty trace total_frames must be 0");
  Expect(stats.p50_ms == 0.0, "empty trace p50 must be 0");
  Expect(stats.p90_ms == 0.0, "empty trace p90 must be 0");
  Expect(stats.p95_ms == 0.0, "empty trace p95 must be 0");
  Expect(stats.p99_ms == 0.0, "empty trace p99 must be 0");
  Expect(stats.hitch_count == 0, "empty trace hitch_count must be 0");
}

void TestSingleFrame() {
  riva::NormalizedTrace trace("single");
  riva::Frame frame;
  frame.index = 0;
  frame.start_time_us = 0;
  frame.duration_ms = 16.0;
  frame.game_thread_ms = 10.0;
  (void)trace.AddFrame(std::move(frame));

  auto stats = riva::ComputeTraceStatistics(trace);
  Expect(stats.total_frames == 1, "single frame total must be 1");
  Expect(stats.p50_ms == 16.0, "single frame p50 must equal the frame");
  Expect(stats.p99_ms == 16.0, "single frame p99 must equal the frame");
  Expect(stats.min_ms == 16.0, "single frame min must equal the frame");
  Expect(stats.max_ms == 16.0, "single frame max must equal the frame");
  Expect(stats.mean_ms == 16.0, "single frame mean must equal the frame");
  Expect(stats.hitch_count == 0, "16ms frame should not be a hitch");
}

void TestUniformTrace() {
  auto trace = MakeUniformTrace(100, 16.0);
  auto stats = riva::ComputeTraceStatistics(trace);

  Expect(stats.total_frames == 100, "uniform trace total must be 100");
  Expect(ApproxEqual(stats.p50_ms, 16.0), "uniform p50 must be 16.0");
  Expect(ApproxEqual(stats.p90_ms, 16.0), "uniform p90 must be 16.0");
  Expect(ApproxEqual(stats.p95_ms, 16.0), "uniform p95 must be 16.0");
  Expect(ApproxEqual(stats.p99_ms, 16.0), "uniform p99 must be 16.0");
  Expect(ApproxEqual(stats.mean_ms, 16.0), "uniform mean must be 16.0");
  Expect(stats.hitch_count == 0, "uniform 16ms trace should have zero hitches");
  Expect(ApproxEqual(stats.hitch_percentage, 0.0), "hitch_percentage must be 0");
}

void TestPercentilesWithSpikes() {
  // 100 frames: 95 at 16ms, 5 at 50ms
  // sorted: [16 x 95, 50 x 5]
  // P50 = 16, P90 = 16, P95 ~ boundary of 16/50, P99 = 50
  riva::NormalizedTrace trace("spiky");
  for (std::size_t i = 0; i < 100; ++i) {
    riva::Frame frame;
    frame.index = i;
    frame.start_time_us = static_cast<std::uint64_t>(i * 16000);
    frame.duration_ms = (i >= 95) ? 50.0 : 16.0;
    frame.game_thread_ms = frame.duration_ms * 0.625;
    frame.render_thread_ms = frame.duration_ms * 0.5;
    frame.gpu_ms = frame.duration_ms * 0.7;
    (void)trace.AddFrame(std::move(frame));
  }

  auto stats = riva::ComputeTraceStatistics(trace);

  Expect(stats.total_frames == 100, "spiky total must be 100");
  Expect(ApproxEqual(stats.p50_ms, 16.0), "spiky p50 must be 16.0");
  Expect(ApproxEqual(stats.p90_ms, 16.0), "spiky p90 must be 16.0");
  // P95 = index 94.05, which is between 16.0 (index 94) and 50.0 (index 95)
  Expect(stats.p95_ms >= 16.0 && stats.p95_ms <= 50.0, "spiky p95 must be between 16 and 50");
  Expect(ApproxEqual(stats.p99_ms, 50.0), "spiky p99 must be 50.0");
  Expect(stats.min_ms == 16.0, "spiky min must be 16.0");
  Expect(stats.max_ms == 50.0, "spiky max must be 50.0");
  Expect(stats.hitch_count == 5, "spiky must have 5 hitches");
  Expect(ApproxEqual(stats.hitch_percentage, 5.0), "spiky hitch percentage must be 5.0");
}

void TestPerMetricP95() {
  auto trace = MakeUniformTrace(100, 16.0);
  auto stats = riva::ComputeTraceStatistics(trace);

  Expect(ApproxEqual(stats.game_thread_p95_ms, 16.0 * 0.625), "game_thread P95 must match ratio");
  Expect(ApproxEqual(stats.render_thread_p95_ms, 16.0 * 0.5), "render_thread P95 must match ratio");
  Expect(ApproxEqual(stats.gpu_p95_ms, 16.0 * 0.7), "gpu P95 must match ratio");
}

void TestHitchThresholdOverride() {
  auto trace = MakeUniformTrace(10, 20.0);  // All 20ms frames
  auto stats = riva::ComputeTraceStatistics(trace, 33.333);
  Expect(stats.hitch_count == 0, "20ms frames should not hitch at 33ms threshold");

  auto stats_strict = riva::ComputeTraceStatistics(trace, 16.667);
  Expect(stats_strict.hitch_count == 10, "20ms frames should all hitch at 16.7ms threshold");
}

}  // namespace

int main() {
  TestEmptyTrace();
  TestSingleFrame();
  TestUniformTrace();
  TestPercentilesWithSpikes();
  TestPerMetricP95();
  TestHitchThresholdOverride();
  std::cout << "All trace statistics tests passed successfully!\n";
  return 0;
}

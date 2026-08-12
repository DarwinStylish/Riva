#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "riva/normalized_trace.hpp"
#include "riva/trace_comparator.hpp"

namespace riva {
namespace {

ResolvedFinding MakePrimaryFinding(std::string id) {
  ResolvedFinding r;
  r.finding.id = id;
  r.role = FindingRole::kPrimary;
  return r;
}

ResolvedFinding MakeSecondaryFinding(std::string id) {
  ResolvedFinding r;
  r.finding.id = id;
  r.role = FindingRole::kSecondary;
  return r;
}

void AssertEq(std::size_t actual, std::size_t expected, const std::string& msg) {
  if (actual != expected) {
    std::cerr << "Assertion failed: " << msg << ". Expected " << expected << ", got " << actual << "\n";
    std::exit(1);
  }
}

void AssertTrue(bool condition, const std::string& msg) {
  if (!condition) {
    std::cerr << "Assertion failed: " << msg << "\n";
    std::exit(1);
  }
}

bool ApproxEqual(double a, double b, double epsilon = 0.1) {
  return std::fabs(a - b) < epsilon;
}

// Helper: build a trace with uniform frame times
NormalizedTrace MakeTrace(const std::string& name, std::size_t count, double duration_ms) {
  NormalizedTrace trace(name);
  for (std::size_t i = 0; i < count; ++i) {
    Frame frame;
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

void TestDetectsRegressionsAndImprovements() {
  auto baseline_trace = MakeTrace("baseline", 10, 16.0);
  auto new_trace_data = MakeTrace("new", 10, 16.0);

  AnalysisResult baseline;
  baseline.findings = {
    MakePrimaryFinding("STUT_GC"),
    MakePrimaryFinding("STUT_STREAMING_IO"),
    MakeSecondaryFinding("STUT_CPU_GT") // Should be ignored
  };

  AnalysisResult new_result;
  new_result.findings = {
    MakePrimaryFinding("STUT_GC"), // Unchanged
    MakePrimaryFinding("STUT_SHADER_COMPILE"), // Regression
    MakeSecondaryFinding("STUT_CPU_RT") // Should be ignored
  };

  DefaultTraceComparator comparator;
  auto output = comparator.Compare(baseline_trace, baseline, new_trace_data, new_result);

  AssertEq(output.unchanged.size(), 1, "Should have 1 unchanged finding");
  AssertTrue(output.unchanged[0].finding.id == "STUT_GC", "GC is unchanged");

  AssertEq(output.regressions.size(), 1, "Should have 1 regression");
  AssertTrue(output.regressions[0].finding.id == "STUT_SHADER_COMPILE", "Shader compile is regression");

  AssertEq(output.improvements.size(), 1, "Should have 1 improvement");
  AssertTrue(output.improvements[0].finding.id == "STUT_STREAMING_IO", "Streaming IO is improvement");
}

void TestMetricDeltas() {
  auto baseline_trace = MakeTrace("baseline", 100, 16.0);
  auto new_trace_data = MakeTrace("new", 100, 20.0);

  AnalysisResult baseline;
  AnalysisResult new_result;

  DefaultTraceComparator comparator;
  auto output = comparator.Compare(baseline_trace, baseline, new_trace_data, new_result);

  // Verify statistics were computed
  AssertTrue(ApproxEqual(output.statistics.baseline.p50_ms, 16.0), "baseline P50 must be 16.0");
  AssertTrue(ApproxEqual(output.statistics.new_trace.p50_ms, 20.0), "new P50 must be 20.0");

  // Verify metric deltas exist
  AssertTrue(!output.statistics.metric_deltas.empty(), "metric deltas must not be empty");

  // Find the P50 delta
  bool found_p50 = false;
  for (const auto& md : output.statistics.metric_deltas) {
    if (md.metric_name == "P50 Frame Time") {
      found_p50 = true;
      AssertTrue(ApproxEqual(md.baseline_value, 16.0), "P50 baseline must be 16.0");
      AssertTrue(ApproxEqual(md.new_value, 20.0), "P50 new must be 20.0");
      AssertTrue(ApproxEqual(md.delta, 4.0), "P50 delta must be +4.0");
      AssertTrue(ApproxEqual(md.delta_percent, 25.0, 0.5), "P50 delta % must be ~25%");
      AssertTrue(md.bRegressed, "P50 must be flagged as regressed");
      break;
    }
  }
  AssertTrue(found_p50, "P50 metric delta must exist");

  // Overall regression
  AssertTrue(output.statistics.bOverallRegressed, "overall must be flagged as regressed");
}

void TestNoRegressionOnIdenticalTraces() {
  auto trace1 = MakeTrace("trace1", 50, 16.0);
  auto trace2 = MakeTrace("trace2", 50, 16.0);

  AnalysisResult result1;
  AnalysisResult result2;

  DefaultTraceComparator comparator;
  auto output = comparator.Compare(trace1, result1, trace2, result2);

  // All deltas should be zero, no regression
  for (const auto& md : output.statistics.metric_deltas) {
    AssertTrue(!md.bRegressed, ("No regression expected for: " + md.metric_name).c_str());
    AssertTrue(ApproxEqual(md.delta, 0.0, 0.01), ("Delta must be zero for: " + md.metric_name).c_str());
  }
  AssertTrue(!output.statistics.bOverallRegressed, "overall must not be regressed for identical traces");
}

}  // namespace
}  // namespace riva

int main() {
  riva::TestDetectsRegressionsAndImprovements();
  riva::TestMetricDeltas();
  riva::TestNoRegressionOnIdenticalTraces();
  std::cout << "Trace comparator tests passed!\n";
  return 0;
}

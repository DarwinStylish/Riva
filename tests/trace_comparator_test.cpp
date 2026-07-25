#include <string>
#include <vector>
#include <iostream>

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

void TestDetectsRegressionsAndImprovements() {
  AnalysisResult baseline;
  baseline.findings = {
    MakePrimaryFinding("STUT_GC"),
    MakePrimaryFinding("STUT_STREAMING_IO"),
    MakeSecondaryFinding("STUT_CPU_GT") // Should be ignored
  };

  AnalysisResult new_trace;
  new_trace.findings = {
    MakePrimaryFinding("STUT_GC"), // Unchanged
    MakePrimaryFinding("STUT_SHADER_COMPILE"), // Regression
    MakeSecondaryFinding("STUT_CPU_RT") // Should be ignored
  };

  DefaultTraceComparator comparator;
  auto output = comparator.Compare(baseline, new_trace);
  
  AssertEq(output.unchanged.size(), 1, "Should have 1 unchanged finding");
  AssertTrue(output.unchanged[0].finding.id == "STUT_GC", "GC is unchanged");

  AssertEq(output.regressions.size(), 1, "Should have 1 regression");
  AssertTrue(output.regressions[0].finding.id == "STUT_SHADER_COMPILE", "Shader compile is regression");

  AssertEq(output.improvements.size(), 1, "Should have 1 improvement");
  AssertTrue(output.improvements[0].finding.id == "STUT_STREAMING_IO", "Streaming IO is improvement");
}

} // namespace
} // namespace riva

int main() {
  riva::TestDetectsRegressionsAndImprovements();
  std::cout << "Trace comparator tests passed!\n";
  return 0;
}

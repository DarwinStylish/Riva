#include "riva/correlation_resolver.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "riva/analysis_result.hpp"

namespace riva {
namespace {

ResolvedFinding MakeFinding(const std::string& id, std::uint64_t start_us, FindingRole role) {
  ResolvedFinding r;
  r.finding.id = id;
  r.finding.title = "Spike";
  r.finding.time_window_start_us = start_us;
  r.finding.time_window_end_us = start_us + 1000;
  r.finding.confidence = 0.5;
  r.role = role;
  return r;
}

void AssertEq(std::size_t actual, std::size_t expected, const std::string& msg) {
  if (actual != expected) {
    std::cerr << "Assertion failed: " << msg << ". Expected " << expected << ", got " << actual
              << "\n";
    std::exit(1);
  }
}

void AssertEq(const std::string& actual, const std::string& expected, const std::string& msg) {
  if (actual != expected) {
    std::cerr << "Assertion failed: " << msg << ". Expected " << expected << ", got " << actual
              << "\n";
    std::exit(1);
  }
}

void TestClustersPrimaryFindings() {
  std::vector<ResolvedFinding> input = {MakeFinding("GC", 1000, FindingRole::kPrimary),
                                        MakeFinding("GC", 2000, FindingRole::kPrimary),
                                        MakeFinding("GC", 3000, FindingRole::kPrimary),
                                        MakeFinding("GC", 10000000, FindingRole::kPrimary)};

  CorrelationResolverConfig config;
  config.min_cluster_size = 3;
  config.max_cluster_window_us = 5000;

  DefaultCorrelationResolver resolver(config);
  auto output = resolver.Resolve(input);

  AssertEq(output.size(), 2, "Should have 2 findings (1 cluster, 1 standalone)");
  AssertEq(output[0].finding.title, "Cluster of: Spike", "First finding should be cluster");
  AssertEq(output[0].finding.time_window_start_us, 1000, "Start time should be 1000");
  AssertEq(output[0].finding.time_window_end_us, 4000, "End time should be 4000");
  AssertEq(output[1].finding.title, "Spike", "Second finding should be standalone");
}

void TestIgnoresSecondaryFindings() {
  std::vector<ResolvedFinding> input = {MakeFinding("GC", 1000, FindingRole::kSecondary),
                                        MakeFinding("GC", 2000, FindingRole::kSecondary),
                                        MakeFinding("GC", 3000, FindingRole::kSecondary)};

  DefaultCorrelationResolver resolver;
  auto output = resolver.Resolve(input);

  AssertEq(output.size(), 3, "Should have 3 findings (none clustered)");
  AssertEq(output[0].finding.title, "Spike", "Title should remain unchanged");
}

}  // namespace
}  // namespace riva

int main() {
  riva::TestClustersPrimaryFindings();
  riva::TestIgnoresSecondaryFindings();
  std::cout << "Correlation resolver tests passed!\n";
  return 0;
}

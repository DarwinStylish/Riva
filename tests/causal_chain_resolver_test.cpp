#include "riva/causal_chain_resolver.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace riva {
namespace {

Finding MakeFinding(const std::string& id, std::size_t frame_index, double confidence) {
  Finding r;
  r.id = id;
  r.frame_index = frame_index;
  r.confidence = confidence;
  return r;
}

void AssertEq(std::size_t actual, std::size_t expected, const std::string& msg) {
  if (actual != expected) {
    std::cerr << "Assertion failed: " << msg << ". Expected " << expected << ", got " << actual
              << "\n";
    std::exit(1);
  }
}

void AssertTrue(bool condition, const std::string& msg) {
  if (!condition) {
    std::cerr << "Assertion failed: " << msg << "\n";
    std::exit(1);
  }
}

void TestResolvesCausalChain() {
  std::vector<Finding> input = {
      MakeFinding("STUT_GC", 1, 0.6),
      MakeFinding("STUT_CPU_GT", 1, 0.8)  // Symptom has higher initial confidence
  };

  DefaultCausalChainResolver resolver;
  auto output = resolver.Resolve(input);

  AssertEq(output.size(), 2, "Should have 2 findings");

  // They should be linked
  AssertTrue(output[0].related_finding_ids.size() == 1, "GC should link to CPU_GT");
  AssertTrue(output[0].related_finding_ids[0] == "STUT_CPU_GT", "GC linked to CPU_GT");
  AssertTrue(output[1].related_finding_ids.size() == 1, "CPU_GT should link to GC");
  AssertTrue(output[1].related_finding_ids[0] == "STUT_GC", "CPU_GT linked to GC");

  // GC confidence should be boosted above CPU_GT
  AssertTrue(output[0].confidence > output[1].confidence,
             "likely-cause candidate confidence should be higher than symptom");
}

void TestIgnoresDifferentFrames() {
  std::vector<Finding> input = {MakeFinding("STUT_GC", 1, 0.6), MakeFinding("STUT_CPU_GT", 2, 0.8)};

  DefaultCausalChainResolver resolver;
  auto output = resolver.Resolve(input);

  AssertTrue(output[0].related_finding_ids.empty(), "Should not link across different frames");
  AssertTrue(output[1].related_finding_ids.empty(), "Should not link across different frames");
}

}  // namespace
}  // namespace riva

int main() {
  riva::TestResolvesCausalChain();
  riva::TestIgnoresDifferentFrames();
  std::cout << "Causal chain resolver tests passed!\n";
  return 0;
}

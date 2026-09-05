#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "riva/analysis_engine.hpp"
#include "riva/builtin_signatures.hpp"
#include "riva/trace_synthesizer.hpp"

namespace {

void Expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << "\n";
    std::exit(1);
  }
}

// Map pathology type to the expected primary signature ID.
std::string ExpectedSignatureId(riva::EPathologyType type) {
  switch (type) {
    case riva::EPathologyType::kShaderCompile:
      return "STUT_SHADER_COMPILE";
    case riva::EPathologyType::kPsoMiss:
      return "STUT_PSO_MISS";
    case riva::EPathologyType::kStreamingIo:
      return "STUT_STREAMING_IO";
    case riva::EPathologyType::kGarbageCollection:
      return "STUT_GC";
    case riva::EPathologyType::kRhiSync:
      return "STUT_RHI_SYNC";
    case riva::EPathologyType::kCpuGameThread:
      return "STUT_CPU_GT";
    case riva::EPathologyType::kCpuRenderThread:
      return "STUT_CPU_RT";
    case riva::EPathologyType::kGpuVarianceLumen:
      return "STUT_GPU_VARIANCE_LUMEN_VSM";
    case riva::EPathologyType::kNone:
      return "";
  }
  return "";
}

// Map pathology type to expected affected_system.
std::string ExpectedSystem(riva::EPathologyType type) {
  switch (type) {
    case riva::EPathologyType::kShaderCompile:
      return "Rendering";
    case riva::EPathologyType::kPsoMiss:
      return "Rendering";
    case riva::EPathologyType::kStreamingIo:
      return "Loading";
    case riva::EPathologyType::kGarbageCollection:
      return "Memory";
    case riva::EPathologyType::kRhiSync:
      return "Rendering";
    case riva::EPathologyType::kCpuGameThread:
      return "CPU";
    case riva::EPathologyType::kCpuRenderThread:
      return "CPU";
    case riva::EPathologyType::kGpuVarianceLumen:
      return "Rendering";
    case riva::EPathologyType::kNone:
      return "";
  }
  return "";
}

std::string PathologyName(riva::EPathologyType type) {
  switch (type) {
    case riva::EPathologyType::kShaderCompile:
      return "ShaderCompile";
    case riva::EPathologyType::kPsoMiss:
      return "PsoMiss";
    case riva::EPathologyType::kStreamingIo:
      return "StreamingIo";
    case riva::EPathologyType::kGarbageCollection:
      return "GarbageCollection";
    case riva::EPathologyType::kRhiSync:
      return "RhiSync";
    case riva::EPathologyType::kCpuGameThread:
      return "CpuGameThread";
    case riva::EPathologyType::kCpuRenderThread:
      return "CpuRenderThread";
    case riva::EPathologyType::kGpuVarianceLumen:
      return "GpuVarianceLumen";
    case riva::EPathologyType::kNone:
      return "None";
  }
  return "Unknown";
}

void ValidatePathology(riva::EPathologyType type) {
  const std::string name = PathologyName(type);
  const std::string expected_id = ExpectedSignatureId(type);
  const std::string expected_system = ExpectedSystem(type);
  const std::size_t injection_frame = 60;

  // Generate synthetic trace
  riva::FTraceSynthesizerConfig config;
  config.source_name = "pathology-validation-" + name;
  config.frame_count = 120;
  config.baseline_frame_ms = 16.0;
  config.baseline_jitter_ms = 0.3;

  riva::FPathologyInjection injection;
  injection.type = type;
  injection.frame_index = injection_frame;
  injection.spike_ms = 55.0;
  config.injections.push_back(injection);

  auto synthesized = riva::FTraceSynthesizer::Generate(config);
  Expect(synthesized.status.ok(), (name + ": synthetic fixture generation must succeed").c_str());

  // Run full analysis pipeline
  auto signatures = riva::CreateBuiltinSignatures();
  riva::AnalysisEngine engine(std::move(signatures));
  auto analysis = engine.Analyze(synthesized.trace);

  // Verify detection
  Expect(!analysis.findings.empty(), (name + ": analysis must produce findings").c_str());

  // Find the expected signature in findings
  bool found = false;
  for (const auto& rf : analysis.findings) {
    if (rf.finding.id == expected_id) {
      found = true;

      // Verify frame index
      Expect(rf.finding.frame_index == injection_frame,
             (name + ": finding must point to injected frame " + std::to_string(injection_frame))
                 .c_str());

      // Verify confidence is reasonable
      Expect(rf.finding.confidence >= 0.60,
             (name + ": finding confidence must be >= 0.60").c_str());

      // Verify affected_system
      Expect(rf.finding.affected_system == expected_system,
             (name + ": affected_system must be " + expected_system + " but got " +
              rf.finding.affected_system)
                 .c_str());

      // Verify evidence exists
      Expect(!rf.finding.evidence.empty(), (name + ": finding must have evidence").c_str());

      break;
    }
  }

  Expect(found,
         (name + ": expected signature " + expected_id + " not found in analysis results").c_str());

  std::cout << "  [PASS] " << name << " -> " << expected_id << "\n";
}

void TestStatisticsAndScoreArePopulated() {
  riva::FTraceSynthesizerConfig config;
  config.source_name = "stats-check";
  config.frame_count = 100;
  config.baseline_frame_ms = 16.0;
  config.baseline_jitter_ms = 0.3;
  config.injections.push_back({riva::EPathologyType::kShaderCompile, 50, 55.0, ""});

  auto synthesized = riva::FTraceSynthesizer::Generate(config);
  Expect(synthesized.status.ok(), "statistics fixture generation must succeed");
  auto signatures = riva::CreateBuiltinSignatures();
  riva::AnalysisEngine engine(std::move(signatures));
  auto analysis = engine.Analyze(synthesized.trace);

  // Verify statistics are populated
  Expect(analysis.statistics.total_frames == 100, "statistics.total_frames must be 100");
  Expect(analysis.statistics.p50_ms > 0.0, "statistics.p50_ms must be > 0");
  Expect(analysis.statistics.p95_ms > 0.0, "statistics.p95_ms must be > 0");
  Expect(analysis.statistics.hitch_count >= 1, "statistics must detect at least 1 hitch");

  // Verify score is populated
  Expect(analysis.score.overall > 0.0 && analysis.score.overall <= 100.0,
         "score.overall must be in (0, 100]");
  Expect(!analysis.score.overall_grade.empty(), "score.overall_grade must be populated");
  Expect(!analysis.score.subsystems.empty(), "score.subsystems must be populated");
}

}  // namespace

int main() {
  std::cout << "Pathology Validation Suite\n";
  std::cout << "=========================\n";

  // Validate all 8 pathology types
  ValidatePathology(riva::EPathologyType::kShaderCompile);
  ValidatePathology(riva::EPathologyType::kPsoMiss);
  ValidatePathology(riva::EPathologyType::kStreamingIo);
  ValidatePathology(riva::EPathologyType::kGarbageCollection);
  ValidatePathology(riva::EPathologyType::kRhiSync);
  ValidatePathology(riva::EPathologyType::kCpuGameThread);
  ValidatePathology(riva::EPathologyType::kCpuRenderThread);
  ValidatePathology(riva::EPathologyType::kGpuVarianceLumen);

  std::cout << "\n";

  // Verify statistics and score enrichment
  TestStatisticsAndScoreArePopulated();

  std::cout << "All pathology validation tests passed successfully!\n";
  return 0;
}

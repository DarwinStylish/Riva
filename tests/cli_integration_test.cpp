#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "riva/analysis_engine.hpp"
#include "riva/builtin_signatures.hpp"
#include "riva/json_trace_adapter.hpp"
#include "riva/report_engine.hpp"
#include "riva/status.hpp"
#include "riva/trace_adapter_registry.hpp"

namespace {

void Expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << "\n";
    std::exit(1);
  }
}

bool Contains(const std::string& text, const std::string& substring) {
  return text.find(substring) != std::string::npos;
}

std::string ResolveSamplePath(const std::string& filename) {
  std::string p1 = "samples/" + filename;
  std::string p2 = "../samples/" + filename;
  std::ifstream f1(p1);
  if (f1.is_open()) {
    return p1;
  }
  std::ifstream f2(p2);
  if (f2.is_open()) {
    return p2;
  }
  return p1;
}

void TestSampleTrace(const std::string& filename,
                     const std::string& expected_source,
                     const std::string& expected_classification,
                     const std::string& expected_id,
                     const std::string& expected_evidence) {
  const std::string path = ResolveSamplePath(filename);
  riva::TraceAdapterRegistry registry;
  registry.Register(std::make_unique<riva::JsonTraceAdapter>());

  const auto load_result = registry.Load(path);
  Expect(load_result.status.ok(), ("Should load sample trace: " + filename).c_str());
  Expect(load_result.trace.has_value(), ("Loaded trace must have value: " + filename).c_str());
  Expect(load_result.trace->frame_count() == 3, ("Sample trace must have 3 frames: " + filename).c_str());

  riva::AnalysisEngine engine(riva::CreateBuiltinSignatures());
  const auto analysis_result = engine.Analyze(*load_result.trace);
  Expect(!analysis_result.findings.empty(), ("Analysis Engine must detect findings: " + filename).c_str());
  Expect(analysis_result.total_frames_analyzed == 3, ("Total frames analyzed must be 3: " + filename).c_str());
  Expect(analysis_result.hitch_count == 1, ("Hitch count must be 1: " + filename).c_str());
  Expect(analysis_result.source_name == expected_source, ("Source name must match: " + filename).c_str());
  Expect(analysis_result.findings[0].finding.title == expected_classification,
         ("Primary classification must match: " + filename).c_str());
  Expect(analysis_result.findings[0].finding.id == expected_id, ("Primary finding ID must match: " + filename).c_str());

  riva::FReportOptions options;
  std::string markdown;
  const auto md_status = riva::FReportEngine::GenerateReport(
      riva::EReportFormat::kMarkdown, analysis_result, options, markdown);
  Expect(md_status.ok(), ("Markdown GenerateReport must succeed: " + filename).c_str());
  Expect(Contains(markdown, "# Riva Performance Diagnostic Report"), ("Markdown must contain header: " + filename).c_str());
  Expect(Contains(markdown, expected_classification), ("Markdown must contain classification: " + filename).c_str());
  if (!expected_evidence.empty()) {
    Expect(Contains(markdown, expected_evidence), ("Markdown must contain evidence: " + filename).c_str());
  }

  std::string json;
  const auto json_status = riva::FReportEngine::GenerateReport(
      riva::EReportFormat::kJson, analysis_result, options, json);
  Expect(json_status.ok(), ("JSON GenerateReport must succeed: " + filename).c_str());
  Expect(Contains(json, "\"executive_summary\": {"), ("JSON must contain executive summary: " + filename).c_str());
  Expect(Contains(json, "\"total_frames_analyzed\": 3"), ("JSON must contain total frames: " + filename).c_str());
  Expect(Contains(json, "\"hitch_count\": 1"), ("JSON must contain hitch count: " + filename).c_str());
  Expect(Contains(json, expected_id), ("JSON must contain ID: " + filename).c_str());
}

void TestAnalyzeMissingTraceFile() {
  riva::TraceAdapterRegistry registry;
  registry.Register(std::make_unique<riva::JsonTraceAdapter>());

  const auto load_result = registry.Load("non_existent_trace.json");
  Expect(!load_result.status.ok(), "Loading missing trace file must fail");
  Expect(!load_result.trace.has_value(), "Missing trace file must return no trace");
}

}  // namespace

int main() {
  TestSampleTrace("spike_shader_compile.json", "spike_shader_compile_sample", "Shader compilation stall",
                  "STUT_SHADER_COMPILE", "ShaderCompileWorker blocked frame");
  TestSampleTrace("spike_pso_miss.json", "spike_pso_miss_sample", "Pipeline state object miss",
                  "STUT_PSO_MISS", "PSO compile draw call wait");
  TestSampleTrace("spike_streaming_io.json", "spike_streaming_io_sample", "Streaming or IO stall",
                  "STUT_STREAMING_IO", "Asset loading Zen IO dispatcher stall");
  TestSampleTrace("spike_cpu_game_thread.json", "spike_cpu_game_thread_sample", "Game thread CPU spike",
                  "STUT_CPU_GT", "game_thread_ms");
  TestSampleTrace("spike_cpu_render_thread.json", "spike_cpu_render_thread_sample", "Render thread CPU spike",
                  "STUT_CPU_RT", "render_thread_ms");
  TestSampleTrace("spike_rhi_sync.json", "spike_rhi_sync_sample", "RHI synchronization stall",
                  "STUT_RHI_SYNC", "Wait for RHI presentation buffer");
  TestSampleTrace("spike_garbage_collection.json", "spike_garbage_collection_sample", "Garbage collection stall",
                  "STUT_GC", "Collect garbage mark");
  TestSampleTrace("spike_gpu_variance_lumen.json", "spike_gpu_variance_lumen_sample",
                  "GPU variance from Lumen or virtual shadow maps", "STUT_GPU_VARIANCE_LUMEN_VSM",
                  "gpu_ms");

  TestAnalyzeMissingTraceFile();
  std::cout << "All CLI integration tests passed successfully across 8 sample trace datasets!\n";
  return 0;
}


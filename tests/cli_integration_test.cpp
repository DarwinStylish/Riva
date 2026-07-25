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

std::string ResolveSamplePath() {
  std::string p1 = "samples/spike_shader_compile.json";
  std::string p2 = "../samples/spike_shader_compile.json";
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

void TestAnalyzeSampleTraceMarkdown() {
  const std::string path = ResolveSamplePath();
  riva::TraceAdapterRegistry registry;
  registry.Register(std::make_unique<riva::JsonTraceAdapter>());

  const auto load_result = registry.Load(path);
  Expect(load_result.status.ok(), "JsonTraceAdapter should successfully load sample trace");
  Expect(load_result.trace.has_value(), "Loaded trace must have value");
  Expect(load_result.trace->frame_count() == 3, "Sample trace must have 3 frames");

  riva::AnalysisEngine engine(riva::CreateBuiltinSignatures());
  const auto analysis_result = engine.Analyze(*load_result.trace);
  Expect(!analysis_result.findings.empty(), "Analysis Engine must detect findings in sample trace");
  Expect(analysis_result.total_frames_analyzed == 3, "Total frames analyzed must be 3");
  Expect(analysis_result.hitch_count == 1, "Hitch count must be 1");
  Expect(analysis_result.source_name == "spike_shader_compile_sample", "Source name must match");

  riva::FReportOptions options;
  std::string markdown;
  const auto report_status = riva::FReportEngine::GenerateReport(
      riva::EReportFormat::kMarkdown, analysis_result, options, markdown);
  Expect(report_status.ok(), "GenerateReport for Markdown must succeed");
  Expect(!markdown.empty(), "Markdown output must not be empty");
  Expect(Contains(markdown, "# Riva Performance Diagnostic Report"), "Markdown must contain header");
  Expect(Contains(markdown, "Shader compilation stall"), "Markdown must contain primary stall classification");
  Expect(Contains(markdown, "ShaderCompileWorker blocked frame"), "Markdown must contain evidence event");
  Expect(Contains(markdown, "How to Confirm in Unreal Insights"), "Markdown must contain confirmation steps");
}

void TestAnalyzeSampleTraceJson() {
  const std::string path = ResolveSamplePath();
  riva::TraceAdapterRegistry registry;
  registry.Register(std::make_unique<riva::JsonTraceAdapter>());

  const auto load_result = registry.Load(path);
  Expect(load_result.status.ok(), "JsonTraceAdapter should successfully load sample trace");

  riva::AnalysisEngine engine(riva::CreateBuiltinSignatures());
  const auto analysis_result = engine.Analyze(*load_result.trace);

  riva::FReportOptions options;
  std::string json;
  const auto report_status = riva::FReportEngine::GenerateReport(
      riva::EReportFormat::kJson, analysis_result, options, json);
  Expect(report_status.ok(), "GenerateReport for JSON must succeed");
  Expect(!json.empty(), "JSON output must not be empty");
  Expect(Contains(json, "\"executive_summary\": {"), "JSON must contain executive summary");
  Expect(Contains(json, "\"total_frames_analyzed\": 3"), "JSON must contain total frames");
  Expect(Contains(json, "\"hitch_count\": 1"), "JSON must contain hitch count");
  Expect(Contains(json, "\"primary_stall_classification\": \"Shader compilation stall\""),
         "JSON must contain primary classification");
  Expect(Contains(json, "\"id\": \"STUT_SHADER_COMPILE\""), "JSON must contain finding ID");
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
  TestAnalyzeSampleTraceMarkdown();
  TestAnalyzeSampleTraceJson();
  TestAnalyzeMissingTraceFile();
  std::cout << "All CLI integration tests passed successfully!\n";
  return 0;
}

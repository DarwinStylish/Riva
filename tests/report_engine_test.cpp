#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "riva/analysis_result.hpp"
#include "riva/finding.hpp"
#include "riva/report_engine.hpp"
#include "riva/trace_comparator.hpp"

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

riva::AnalysisResult MakeSampleResult() {
  riva::AnalysisResult result;
  result.source_name = "report-engine-test";
  result.total_frames_analyzed = 120;
  result.hitch_count = 3;

  riva::Finding f1;
  f1.id = "GPU_STALL";
  f1.title = "Heavy GPU Render Stall";
  f1.severity = riva::Severity::kCritical;
  f1.confidence = 0.90;
  f1.frame_index = 42;
  f1.time_window_start_us = 672000;
  f1.time_window_end_us = 722000;
  f1.evidence.push_back(riva::Evidence{"gpu_ms", "50.00 ms"});
  f1.suggested_next_steps.push_back("Check GPU pass breakdown in Unreal Insights.");
  f1.how_to_confirm.push_back("Verify GPU timing lane exceeds 33ms threshold.");

  riva::ResolvedFinding rf1;
  rf1.finding = f1;
  rf1.role = riva::FindingRole::kPrimary;
  rf1.resolution_note = "Selected as primary bottleneck.";
  result.findings.push_back(std::move(rf1));

  riva::Finding f2;
  f2.id = "GC_STALL";
  f2.title = "Garbage Collection Mark Sweep";
  f2.severity = riva::Severity::kWarning;
  f2.confidence = 0.65;
  f2.frame_index = 42;
  f2.time_window_start_us = 675000;
  f2.time_window_end_us = 680000;
  f2.evidence.push_back(riva::Evidence{"gc_ms", "5.00 ms"});
  f2.suggested_next_steps.push_back("Check UObject allocation rate.");
  f2.how_to_confirm.push_back("Look for GarbageCollect event on GameThread.");

  riva::ResolvedFinding rf2;
  rf2.finding = f2;
  rf2.role = riva::FindingRole::kSecondary;
  rf2.resolution_note = "Secondary concurrent stall.";
  result.findings.push_back(std::move(rf2));

  return result;
}

void TestMarkdownReportWithFindings() {
  const auto result = MakeSampleResult();
  riva::FReportOptions options;

  std::string markdown;
  const auto status = riva::FReportEngine::GenerateMarkdownReport(result, options, markdown);
  Expect(status.ok(), "GenerateMarkdownReport should succeed");
  Expect(!markdown.empty(), "Markdown report should not be empty");

  Expect(Contains(markdown, "# Riva Performance Diagnostic Report"), "Should contain title");
  Expect(Contains(markdown, "## Executive Summary"), "Should contain Executive Summary");
  Expect(Contains(markdown, "- **Source**: report-engine-test"), "Should contain source name");
  Expect(Contains(markdown, "- **Total Frames Analyzed**: 120"), "Should contain total frames analyzed");
  Expect(Contains(markdown, "- **Frame Hitch Count**: 3"), "Should contain hitch count");
  Expect(Contains(markdown, "- **Primary Stall Classification**: Heavy GPU Render Stall"),
         "Should contain primary stall classification");

  Expect(Contains(markdown, "## Findings List"), "Should contain Findings List");
  Expect(Contains(markdown, "### 1. [CRITICAL] Heavy GPU Render Stall (ID: GPU_STALL)"),
         "Should contain primary finding header");
  Expect(Contains(markdown, "- **Role**: Primary"), "Should indicate Primary role");
  Expect(Contains(markdown, "- **Confidence**: 90.0%"), "Should format confidence percentage");
  Expect(Contains(markdown, "- **Time Window**: 672000 us - 722000 us"), "Should format time window");
  Expect(Contains(markdown, "- **Resolution Note**: Selected as primary bottleneck."), "Should include resolution note");

  Expect(Contains(markdown, "#### Evidence Breakdown"), "Should contain Evidence Breakdown");
  Expect(Contains(markdown, "- `gpu_ms`: 50.00 ms"), "Should format evidence item");

  Expect(Contains(markdown, "#### Actionable Guidance"), "Should contain Actionable Guidance");
  Expect(Contains(markdown, "- **Suggested Next Steps**:"), "Should contain Suggested Next Steps");
  Expect(Contains(markdown, "1. Check GPU pass breakdown in Unreal Insights."), "Should contain next step text");
  Expect(Contains(markdown, "- **How to Confirm in Unreal Insights**:"), "Should contain How to Confirm section");
  Expect(Contains(markdown, "1. Verify GPU timing lane exceeds 33ms threshold."), "Should contain confirm instruction");
}

void TestJsonReportWithFindings() {
  const auto result = MakeSampleResult();
  riva::FReportOptions options;

  std::string json;
  const auto status = riva::FReportEngine::GenerateJsonReport(result, options, json);
  Expect(status.ok(), "GenerateJsonReport should succeed");
  Expect(!json.empty(), "JSON report should not be empty");

  Expect(Contains(json, "\"report_title\": \"Riva Performance Diagnostic Report\""), "Should contain JSON title");
  Expect(Contains(json, "\"source_name\": \"report-engine-test\""), "Should contain JSON source name");
  Expect(Contains(json, "\"executive_summary\": {"), "Should contain JSON executive summary");
  Expect(Contains(json, "\"total_frames_analyzed\": 120"), "Should contain JSON total frames");
  Expect(Contains(json, "\"hitch_count\": 3"), "Should contain JSON hitch count");
  Expect(Contains(json, "\"primary_stall_classification\": \"Heavy GPU Render Stall\""),
         "Should contain JSON primary stall classification");

  Expect(Contains(json, "\"findings\": ["), "Should contain JSON findings array");
  Expect(Contains(json, "\"id\": \"GPU_STALL\""), "Should contain finding ID in JSON");
  Expect(Contains(json, "\"severity\": \"CRITICAL\""), "Should format severity in JSON");
  Expect(Contains(json, "\"role\": \"Primary\""), "Should format role in JSON");
  Expect(Contains(json, "\"confidence\": 0.900000"), "Should format confidence double in JSON");
  Expect(Contains(json, "\"evidence\": ["), "Should contain evidence array in JSON");
  Expect(Contains(json, "\"label\": \"gpu_ms\""), "Should contain evidence label in JSON");
  Expect(Contains(json, "\"value\": \"50.00 ms\""), "Should contain evidence value in JSON");
  Expect(Contains(json, "\"how_to_confirm\": ["), "Should contain how_to_confirm array in JSON");
  Expect(Contains(json, "\"Verify GPU timing lane exceeds 33ms threshold.\""), "Should contain confirm string in JSON");
}

void TestEmptyFindingsReport() {
  riva::AnalysisResult result;
  result.source_name = "clean-trace";
  result.total_frames_analyzed = 60;
  result.hitch_count = 0;

  riva::FReportOptions options;
  std::string markdown;
  std::string json;

  auto status = riva::FReportEngine::GenerateMarkdownReport(result, options, markdown);
  Expect(status.ok(), "GenerateMarkdownReport for empty result should succeed");
  Expect(Contains(markdown, "No performance stalls or anomalies detected."),
         "Should report no stalls detected in Markdown");
  Expect(Contains(markdown, "- **Primary Stall Classification**: None"),
         "Should classify primary stall as None when empty in Markdown");

  status = riva::FReportEngine::GenerateJsonReport(result, options, json);
  Expect(status.ok(), "GenerateJsonReport for empty result should succeed");
  Expect(Contains(json, "\"primary_stall_classification\": \"None\""),
         "Should classify primary stall as None when empty in JSON");
  Expect(Contains(json, "\"findings\": []"), "Should output empty findings array in JSON");
}

void TestReportOptionsToggles() {
  const auto result = MakeSampleResult();
  riva::FReportOptions options;
  options.include_metadata = false;
  options.include_executive_summary = false;
  options.include_evidence_details = false;
  options.include_actionable_guidance = false;

  std::string markdown;
  std::string json;

  auto status = riva::FReportEngine::GenerateMarkdownReport(result, options, markdown);
  Expect(status.ok(), "GenerateMarkdownReport with toggles should succeed");
  Expect(!Contains(markdown, "# Riva Performance Diagnostic Report"), "Should omit title when metadata false");
  Expect(!Contains(markdown, "## Executive Summary"), "Should omit Executive Summary when false");
  Expect(!Contains(markdown, "#### Evidence Breakdown"), "Should omit Evidence Breakdown when false");
  Expect(!Contains(markdown, "#### Actionable Guidance"), "Should omit Actionable Guidance when false");

  status = riva::FReportEngine::GenerateJsonReport(result, options, json);
  Expect(status.ok(), "GenerateJsonReport with toggles should succeed");
  Expect(!Contains(json, "\"report_title\""), "Should omit report_title when metadata false");
  Expect(!Contains(json, "\"executive_summary\""), "Should omit executive_summary when false");
  Expect(!Contains(json, "\"evidence\""), "Should omit evidence array when false");
  Expect(!Contains(json, "\"suggested_next_steps\""), "Should omit suggested_next_steps when false");
  Expect(!Contains(json, "\"how_to_confirm\""), "Should omit how_to_confirm when false");
}

void TestGenerateReportHelper() {
  const auto result = MakeSampleResult();
  riva::FReportOptions options;

  std::string md_out;
  auto status = riva::FReportEngine::GenerateReport(riva::EReportFormat::kMarkdown, result, options, md_out);
  Expect(status.ok(), "GenerateReport kMarkdown should succeed");
  Expect(Contains(md_out, "## Executive Summary"), "Helper should generate valid Markdown");

  std::string json_out;
  status = riva::FReportEngine::GenerateReport(riva::EReportFormat::kJson, result, options, json_out);
  Expect(status.ok(), "GenerateReport kJson should succeed");
  Expect(Contains(json_out, "\"executive_summary\""), "Helper should generate valid JSON");
}

void TestGenerateComparisonReport() {
  riva::ComparisonResult result;
  
  riva::Finding f1;
  f1.id = "STUT_SHADER_COMPILE";
  f1.title = "Shader Compilation Stall";
  f1.severity = riva::Severity::kCritical;
  f1.confidence = 0.95;
  f1.frame_index = 10;
  
  riva::ResolvedFinding rf1;
  rf1.finding = f1;
  rf1.role = riva::FindingRole::kPrimary;
  result.regressions.push_back(rf1);

  riva::Finding f2;
  f2.id = "STUT_STREAMING_IO";
  f2.title = "Streaming IO Stall";
  f2.severity = riva::Severity::kWarning;
  f2.confidence = 0.80;
  f2.frame_index = 15;
  
  riva::ResolvedFinding rf2;
  rf2.finding = f2;
  rf2.role = riva::FindingRole::kPrimary;
  result.improvements.push_back(rf2);

  riva::FReportOptions options;
  std::string markdown;
  const auto status = riva::FReportEngine::GenerateComparisonReport(result, options, markdown);
  
  Expect(status.ok(), "GenerateComparisonReport should succeed");
  Expect(Contains(markdown, "Trace Comparison"), "Should contain Trace Comparison header");
  Expect(Contains(markdown, "## Executive Summary"), "Should contain Executive Summary");
  Expect(Contains(markdown, "- **Regressions**: 1"), "Should report 1 regression");
  Expect(Contains(markdown, "- **Improvements**: 1"), "Should report 1 improvement");
  Expect(Contains(markdown, "- **Unchanged**: 0"), "Should report 0 unchanged");
  Expect(Contains(markdown, "Shader Compilation Stall"), "Should detail regression finding");
  Expect(Contains(markdown, "Streaming IO Stall"), "Should detail improvement finding");
}

}  // namespace

int main() {
  TestMarkdownReportWithFindings();
  TestJsonReportWithFindings();
  TestEmptyFindingsReport();
  TestReportOptionsToggles();
  TestGenerateReportHelper();
  TestGenerateComparisonReport();
  std::cout << "All report engine tests passed successfully!\n";
  return 0;
}

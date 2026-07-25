#include "riva/report_engine.hpp"

#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

namespace riva {

namespace {

std::string EscapeJsonString(const std::string& input) {
  std::string output;
  output.reserve(input.size() + 8);
  for (char c : input) {
    switch (c) {
      case '"': output += "\\\""; break;
      case '\\': output += "\\\\"; break;
      case '\b': output += "\\b"; break;
      case '\f': output += "\\f"; break;
      case '\n': output += "\\n"; break;
      case '\r': output += "\\r"; break;
      case '\t': output += "\\t"; break;
      default: output += c; break;
    }
  }
  return output;
}

std::string SeverityToString(Severity severity) {
  switch (severity) {
    case Severity::kInfo: return "INFO";
    case Severity::kWarning: return "WARNING";
    case Severity::kCritical: return "CRITICAL";
  }
  return "UNKNOWN";
}

std::string RoleToString(FindingRole role) {
  switch (role) {
    case FindingRole::kPrimary: return "Primary";
    case FindingRole::kSecondary: return "Secondary";
  }
  return "Secondary";
}

std::string FormatConfidence(double confidence) {
  std::ostringstream ss;
  ss << std::fixed << std::setprecision(1) << (confidence * 100.0) << "%";
  return ss.str();
}

std::string FormatDoubleJson(double val) {
  std::ostringstream ss;
  ss << std::fixed << std::setprecision(6) << val;
  return ss.str();
}

std::string GetPrimaryStallClassification(const AnalysisResult& result) {
  for (const auto& resolved : result.findings) {
    if (resolved.role == FindingRole::kPrimary) {
      return resolved.finding.title;
    }
  }
  if (!result.findings.empty()) {
    return result.findings[0].finding.title;
  }
  return "None";
}

}  // namespace

Status FReportEngine::GenerateReport(EReportFormat InFormat,
                                     const AnalysisResult& InResult,
                                     const FReportOptions& InOptions,
                                     std::string& OutReport) {
  switch (InFormat) {
    case EReportFormat::kMarkdown:
      return GenerateMarkdownReport(InResult, InOptions, OutReport);
    case EReportFormat::kJson:
      return GenerateJsonReport(InResult, InOptions, OutReport);
  }
  return Status(StatusCode::kInvalidArgument, "Unsupported report format");
}

Status FReportEngine::GenerateMarkdownReport(const AnalysisResult& InResult,
                                             const FReportOptions& InOptions,
                                             std::string& OutMarkdown) {
  std::ostringstream ss;

  if (InOptions.include_metadata) {
    ss << "# " << InOptions.report_title << "\n\n";
  }

  if (InOptions.include_executive_summary) {
    ss << "## Executive Summary\n";
    if (InOptions.include_metadata && !InResult.source_name.empty()) {
      ss << "- **Source**: " << InResult.source_name << "\n";
    }
    ss << "- **Total Frames Analyzed**: " << InResult.total_frames_analyzed << "\n";
    ss << "- **Frame Hitch Count**: " << InResult.hitch_count << "\n";
    ss << "- **Primary Stall Classification**: " << GetPrimaryStallClassification(InResult) << "\n\n";
  }

  ss << "## Findings List\n\n";

  if (InResult.findings.empty()) {
    ss << "No performance stalls or anomalies detected.\n";
  } else {
    std::size_t index = 1;
    for (const auto& resolved : InResult.findings) {
      const auto& f = resolved.finding;
      ss << "### " << index++ << ". [" << SeverityToString(f.severity) << "] "
         << f.title << " (ID: " << f.id << ")\n";
      ss << "- **Role**: " << RoleToString(resolved.role) << "\n";
      ss << "- **Confidence**: " << FormatConfidence(f.confidence) << "\n";
      ss << "- **Frame Index**: " << f.frame_index << "\n";
      ss << "- **Time Window**: " << f.time_window_start_us << " us - "
         << f.time_window_end_us << " us\n";
      if (!resolved.resolution_note.empty()) {
        ss << "- **Resolution Note**: " << resolved.resolution_note << "\n";
      }
      ss << "\n";

      if (InOptions.include_evidence_details && !f.evidence.empty()) {
        ss << "#### Evidence Breakdown\n";
        for (const auto& ev : f.evidence) {
          ss << "- `" << ev.label << "`: " << ev.value << "\n";
        }
        ss << "\n";
      }

      if (InOptions.include_actionable_guidance) {
        if (!f.suggested_next_steps.empty() || !f.how_to_confirm.empty()) {
          ss << "#### Actionable Guidance\n";
          if (!f.suggested_next_steps.empty()) {
            ss << "- **Suggested Next Steps**:\n";
            std::size_t step_idx = 1;
            for (const auto& step : f.suggested_next_steps) {
              ss << "  " << step_idx++ << ". " << step << "\n";
            }
          }
          if (!f.how_to_confirm.empty()) {
            ss << "- **How to Confirm in Unreal Insights**:\n";
            std::size_t step_idx = 1;
            for (const auto& confirm : f.how_to_confirm) {
              ss << "  " << step_idx++ << ". " << confirm << "\n";
            }
          }
          ss << "\n";
        }
      }

      if (index <= InResult.findings.size()) {
        ss << "---\n\n";
      }
    }
  }

  OutMarkdown = ss.str();
  return Status::Ok();
}

Status FReportEngine::GenerateJsonReport(const AnalysisResult& InResult,
                                         const FReportOptions& InOptions,
                                         std::string& OutJson) {
  std::ostringstream ss;
  ss << "{\n";
  if (InOptions.include_metadata) {
    ss << "  \"report_title\": \"" << EscapeJsonString(InOptions.report_title) << "\",\n";
    ss << "  \"source_name\": \"" << EscapeJsonString(InResult.source_name) << "\",\n";
    ss << "  \"timestamp_format\": \"" << EscapeJsonString(InOptions.timestamp_format) << "\",\n";
    ss << "  \"use_utc_timestamps\": " << (InOptions.use_utc_timestamps ? "true" : "false") << ",\n";
  }

  if (InOptions.include_executive_summary) {
    ss << "  \"executive_summary\": {\n";
    ss << "    \"total_frames_analyzed\": " << InResult.total_frames_analyzed << ",\n";
    ss << "    \"hitch_count\": " << InResult.hitch_count << ",\n";
    ss << "    \"primary_stall_classification\": \""
       << EscapeJsonString(GetPrimaryStallClassification(InResult)) << "\"\n";
    ss << "  },\n";
  }

  ss << "  \"findings\": [";
  if (InResult.findings.empty()) {
    ss << "]\n";
  } else {
    ss << "\n";
    for (std::size_t i = 0; i < InResult.findings.size(); ++i) {
      const auto& resolved = InResult.findings[i];
      const auto& f = resolved.finding;

      ss << "    {\n";
      ss << "      \"id\": \"" << EscapeJsonString(f.id) << "\",\n";
      ss << "      \"title\": \"" << EscapeJsonString(f.title) << "\",\n";
      ss << "      \"severity\": \"" << SeverityToString(f.severity) << "\",\n";
      ss << "      \"role\": \"" << RoleToString(resolved.role) << "\",\n";
      ss << "      \"confidence\": " << FormatDoubleJson(f.confidence) << ",\n";
      ss << "      \"frame_index\": " << f.frame_index << ",\n";
      ss << "      \"time_window_start_us\": " << f.time_window_start_us << ",\n";
      ss << "      \"time_window_end_us\": " << f.time_window_end_us << ",\n";
      ss << "      \"resolution_note\": \"" << EscapeJsonString(resolved.resolution_note) << "\"";

      if (InOptions.include_evidence_details) {
        ss << ",\n      \"evidence\": [";
        if (f.evidence.empty()) {
          ss << "]";
        } else {
          ss << "\n";
          for (std::size_t j = 0; j < f.evidence.size(); ++j) {
            const auto& ev = f.evidence[j];
            ss << "        {\n";
            ss << "          \"label\": \"" << EscapeJsonString(ev.label) << "\",\n";
            ss << "          \"value\": \"" << EscapeJsonString(ev.value) << "\"\n";
            ss << "        }" << (j + 1 < f.evidence.size() ? "," : "") << "\n";
          }
          ss << "      ]";
        }
      }

      if (InOptions.include_actionable_guidance) {
        ss << ",\n      \"suggested_next_steps\": [";
        if (f.suggested_next_steps.empty()) {
          ss << "]";
        } else {
          ss << "\n";
          for (std::size_t j = 0; j < f.suggested_next_steps.size(); ++j) {
            ss << "        \"" << EscapeJsonString(f.suggested_next_steps[j]) << "\""
               << (j + 1 < f.suggested_next_steps.size() ? "," : "") << "\n";
          }
          ss << "      ]";
        }

        ss << ",\n      \"how_to_confirm\": [";
        if (f.how_to_confirm.empty()) {
          ss << "]";
        } else {
          ss << "\n";
          for (std::size_t j = 0; j < f.how_to_confirm.size(); ++j) {
            ss << "        \"" << EscapeJsonString(f.how_to_confirm[j]) << "\""
               << (j + 1 < f.how_to_confirm.size() ? "," : "") << "\n";
          }
          ss << "      ]";
        }
      }

      ss << "\n    }" << (i + 1 < InResult.findings.size() ? "," : "") << "\n";
    }
    ss << "  ]\n";
  }

  ss << "}\n";
  OutJson = ss.str();
  return Status::Ok();
}

}  // namespace riva

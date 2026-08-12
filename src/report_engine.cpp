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

std::string EvidenceClassificationToString(EEvidenceClassification classification) {
  switch (classification) {
    case EEvidenceClassification::kObserved: return "OBSERVED";
    case EEvidenceClassification::kDerived: return "DERIVED";
    case EEvidenceClassification::kCorrelated: return "CORRELATED";
    case EEvidenceClassification::kInferred: return "INFERRED";
    case EEvidenceClassification::kSuspected: return "SUSPECTED";
    case EEvidenceClassification::kRecommended: return "RECOMMENDED";
  }
  return "OBSERVED";
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

  // Performance Summary section
  if (InResult.statistics.total_frames > 0) {
    ss << "## Performance Summary\n";
    ss << "- **Overall Score**: " << std::fixed << std::setprecision(0)
       << InResult.score.overall << " / 100 (" << InResult.score.overall_grade << ")\n";
    ss << "- **P50**: " << std::setprecision(2) << InResult.statistics.p50_ms
       << " ms | **P95**: " << InResult.statistics.p95_ms
       << " ms | **P99**: " << InResult.statistics.p99_ms << " ms\n";
    ss << "- **Hitch Rate**: " << std::setprecision(1) << InResult.statistics.hitch_percentage
       << "% (" << InResult.statistics.hitch_count << " of "
       << InResult.statistics.total_frames << " frames)\n\n";

    if (!InResult.score.subsystems.empty()) {
      ss << "### Subsystem Scores\n";
      ss << "| Subsystem | Score | Grade | Notes |\n";
      ss << "|---|---|---|---|\n";
      for (const auto& sub : InResult.score.subsystems) {
        ss << "| " << sub.name << " | " << std::setprecision(0) << sub.score
           << " | " << sub.grade << " | ";
        if (sub.deductions.empty()) {
          ss << "Within budget";
        } else {
          ss << sub.deductions[0];
        }
        ss << " |\n";
      }
      ss << "\n";
    }
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
      if (!f.affected_thread.empty()) {
        ss << "- **Affected Thread**: " << f.affected_thread << "\n";
      }
      if (!f.affected_system.empty()) {
        ss << "- **Affected System**: " << f.affected_system << "\n";
      }
      if (!resolved.resolution_note.empty()) {
        ss << "- **Resolution Note**: " << resolved.resolution_note << "\n";
      }
      ss << "\n";

      if (InOptions.include_evidence_details && !f.evidence.empty()) {
        ss << "#### Evidence Breakdown\n";
        for (const auto& ev : f.evidence) {
          ss << "- `" << ev.label << "` [" << EvidenceClassificationToString(ev.classification) << "]: " << ev.value << "\n";
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
      ss << "      \"affected_thread\": \"" << EscapeJsonString(f.affected_thread) << "\",\n";
      ss << "      \"affected_system\": \"" << EscapeJsonString(f.affected_system) << "\",\n";
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
            ss << "          \"value\": \"" << EscapeJsonString(ev.value) << "\",\n";
            ss << "          \"classification\": \"" << EvidenceClassificationToString(ev.classification) << "\"\n";
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

  // Insert statistics and score before closing brace
  // We need to rebuild the JSON to include these blocks
  std::string json_str = ss.str();
  // Find the position before the final closing brace
  auto close_pos = json_str.rfind('}');
  if (close_pos != std::string::npos && InResult.statistics.total_frames > 0) {
    std::ostringstream extra;
    // We need a comma after the last existing field
    // Find the content before the closing brace
    auto before_close = json_str.substr(0, close_pos);
    // Check if there's content (needs comma)
    if (!before_close.empty() && before_close.back() != '{' && before_close.back() != ',') {
      // Remove trailing whitespace to add comma
      while (!before_close.empty() && (before_close.back() == '\n' || before_close.back() == ' ')) {
        before_close.pop_back();
      }
      before_close += ",\n";
    }
    extra << "  \"statistics\": {\n";
    extra << "    \"p50_ms\": " << std::fixed << std::setprecision(2) << InResult.statistics.p50_ms << ",\n";
    extra << "    \"p90_ms\": " << InResult.statistics.p90_ms << ",\n";
    extra << "    \"p95_ms\": " << InResult.statistics.p95_ms << ",\n";
    extra << "    \"p99_ms\": " << InResult.statistics.p99_ms << ",\n";
    extra << "    \"min_ms\": " << InResult.statistics.min_ms << ",\n";
    extra << "    \"max_ms\": " << InResult.statistics.max_ms << ",\n";
    extra << "    \"mean_ms\": " << InResult.statistics.mean_ms << ",\n";
    extra << "    \"hitch_count\": " << InResult.statistics.hitch_count << ",\n";
    extra << "    \"hitch_percentage\": " << std::setprecision(1) << InResult.statistics.hitch_percentage << "\n";
    extra << "  },\n";
    extra << "  \"performance_score\": {\n";
    extra << "    \"overall\": " << std::setprecision(1) << InResult.score.overall << ",\n";
    extra << "    \"grade\": \"" << InResult.score.overall_grade << "\",\n";
    extra << "    \"subsystems\": [\n";
    for (std::size_t i = 0; i < InResult.score.subsystems.size(); ++i) {
      const auto& sub = InResult.score.subsystems[i];
      extra << "      {\n";
      extra << "        \"name\": \"" << EscapeJsonString(sub.name) << "\",\n";
      extra << "        \"score\": " << std::setprecision(1) << sub.score << ",\n";
      extra << "        \"grade\": \"" << sub.grade << "\"\n";
      extra << "      }" << (i + 1 < InResult.score.subsystems.size() ? "," : "") << "\n";
    }
    extra << "    ]\n";
    extra << "  }\n";
    json_str = before_close + extra.str() + "}\n";
  }

  OutJson = json_str;
  return Status::Ok();
}

Status FReportEngine::GenerateComparisonReport(const ComparisonResult& InResult,
                                               const FReportOptions& InOptions,
                                               std::string& OutMarkdown) {
  std::ostringstream ss;

  if (InOptions.include_metadata) {
    ss << "# " << InOptions.report_title << " - Trace Comparison\n\n";
  }

  if (InOptions.include_executive_summary) {
    ss << "## Executive Summary\n";
    ss << "- **Regressions**: " << InResult.regressions.size() << "\n";
    ss << "- **Improvements**: " << InResult.improvements.size() << "\n";
    ss << "- **Unchanged**: " << InResult.unchanged.size() << "\n\n";
  }

  auto FormatFindingSection = [&](const std::string& section_title,
                                  const std::vector<ResolvedFinding>& findings,
                                  const std::string& empty_msg) {
    ss << "## " << section_title << "\n\n";
    if (findings.empty()) {
      ss << empty_msg << "\n\n";
      return;
    }

    std::size_t index = 1;
    for (const auto& resolved : findings) {
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

      if (index <= findings.size()) {
        ss << "---\n\n";
      }
    }
  };

  FormatFindingSection("Regressions", InResult.regressions, "No performance regressions detected.");
  FormatFindingSection("Improvements", InResult.improvements, "No performance improvements detected.");
  FormatFindingSection("Unchanged Findings", InResult.unchanged, "No unchanged findings.");

  // Metric Summary table
  if (!InResult.statistics.metric_deltas.empty()) {
    ss << "## Metric Summary\n\n";
    ss << "| Metric | Baseline | New | Delta | Change |\n";
    ss << "|---|---|---|---|---|\n";

    for (const auto& md : InResult.statistics.metric_deltas) {
      std::ostringstream baseline_ss;
      baseline_ss << std::fixed << std::setprecision(2) << md.baseline_value;

      std::ostringstream new_ss;
      new_ss << std::fixed << std::setprecision(2) << md.new_value;

      std::ostringstream delta_ss;
      delta_ss << std::fixed << std::setprecision(2);
      if (md.delta >= 0.0) {
        delta_ss << "+" << md.delta;
      } else {
        delta_ss << md.delta;
      }

      std::ostringstream pct_ss;
      pct_ss << std::fixed << std::setprecision(1);
      if (md.delta_percent >= 0.0) {
        pct_ss << "+" << md.delta_percent << "%";
      } else {
        pct_ss << md.delta_percent << "%";
      }

      // Determine unit suffix
      std::string unit = " ms";
      if (md.metric_name.find("Hitch") != std::string::npos) {
        unit = "%";
      }

      // Regression indicator
      std::string indicator;
      if (md.bRegressed) {
        if (md.delta_percent > 25.0) {
          indicator = " 🔴";
        } else {
          indicator = " ⚠️";
        }
      }

      ss << "| " << md.metric_name
         << " | " << baseline_ss.str() << unit
         << " | " << new_ss.str() << unit
         << " | " << delta_ss.str() << unit
         << " | " << pct_ss.str() << indicator
         << " |\n";
    }
    ss << "\n";
  }

  OutMarkdown = ss.str();
  return Status::Ok();
}

}  // namespace riva

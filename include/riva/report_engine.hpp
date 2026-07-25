#pragma once

#include <string>

#include "riva/analysis_result.hpp"
#include "riva/status.hpp"

namespace riva {

enum class EReportFormat {
  kMarkdown = 0,
  kJson,
};

struct FReportOptions {
  bool include_evidence_details{true};
  bool include_executive_summary{true};
  bool include_actionable_guidance{true};
  bool include_metadata{true};
  bool use_utc_timestamps{true};
  std::string timestamp_format{"ISO8601"};
  std::string report_title{"Riva Performance Diagnostic Report"};
};

class FReportEngine {
 public:
  static Status GenerateReport(EReportFormat InFormat,
                               const AnalysisResult& InResult,
                               const FReportOptions& InOptions,
                               std::string& OutReport);

  static Status GenerateMarkdownReport(const AnalysisResult& InResult,
                                       const FReportOptions& InOptions,
                                       std::string& OutMarkdown);

  static Status GenerateJsonReport(const AnalysisResult& InResult,
                                   const FReportOptions& InOptions,
                                   std::string& OutJson);
};

}  // namespace riva

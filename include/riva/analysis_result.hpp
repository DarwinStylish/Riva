#pragma once

#include <string>
#include <vector>

#include "riva/finding.hpp"

namespace riva {

enum class FindingRole {
  kPrimary = 0,
  kSecondary,
};

struct ResolvedFinding {
  Finding finding;
  FindingRole role{FindingRole::kSecondary};
  std::string resolution_note;
};

struct BudgetStatus {
  bool breached{false};
  std::vector<std::string> breached_metrics;
};

struct AnalysisResult {
  std::vector<ResolvedFinding> findings;
  std::size_t total_frames_analyzed{0};
  std::size_t hitch_count{0};
  std::string source_name;
  BudgetStatus budget_status;
};

}  // namespace riva

#include "riva/trace_comparator.hpp"
#include <set>
#include <string>

namespace riva {

ComparisonResult DefaultTraceComparator::Compare(const AnalysisResult& baseline, const AnalysisResult& new_trace) const {
  ComparisonResult result;

  // We compare by signature ID.
  std::set<std::string> baseline_ids;
  for (const auto& rf : baseline.findings) {
    if (rf.role == FindingRole::kPrimary) {
      baseline_ids.insert(rf.finding.id);
    }
  }

  std::set<std::string> new_ids;
  for (const auto& rf : new_trace.findings) {
    if (rf.role == FindingRole::kPrimary) {
      new_ids.insert(rf.finding.id);
    }
  }

  // Populate regressions (in new, not in baseline)
  for (const auto& rf : new_trace.findings) {
    if (rf.role == FindingRole::kPrimary) {
      if (baseline_ids.find(rf.finding.id) == baseline_ids.end()) {
        result.regressions.push_back(rf);
      } else {
        result.unchanged.push_back(rf); // For unchanged, we return the new trace's findings
      }
    }
  }

  // Populate improvements (in baseline, not in new)
  for (const auto& rf : baseline.findings) {
    if (rf.role == FindingRole::kPrimary) {
      if (new_ids.find(rf.finding.id) == new_ids.end()) {
        result.improvements.push_back(rf);
      }
    }
  }

  return result;
}

}  // namespace riva

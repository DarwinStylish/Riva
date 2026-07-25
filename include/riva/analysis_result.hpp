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

struct AnalysisResult {
  std::vector<ResolvedFinding> findings;
};

}  // namespace riva

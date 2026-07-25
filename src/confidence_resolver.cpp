#include "riva/confidence_resolver.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace riva {
namespace {

int SeverityRank(Severity severity) {
  switch (severity) {
    case Severity::kCritical:
      return 3;
    case Severity::kWarning:
      return 2;
    case Severity::kInfo:
      return 1;
  }

  return 0;
}

bool HasConfirmationGuidance(const Finding& finding) {
  return !finding.how_to_confirm.empty();
}

bool HasEvidence(const Finding& finding) {
  return !finding.evidence.empty();
}

std::string RoleNote(FindingRole role) {
  if (role == FindingRole::kPrimary) {
    return "Primary finding selected by calibrated confidence, severity, and evidence.";
  }

  return "Secondary finding retained because it may contribute to the same spike window.";
}

}  // namespace

ConfidenceResolver::ConfidenceResolver(ConfidenceResolverConfig config)
    : config_(config) {}

AnalysisResult ConfidenceResolver::Resolve(std::vector<Finding> findings) const {
  AnalysisResult result;

  for (auto& finding : findings) {
    finding.confidence = Calibrate(finding);
  }

  findings.erase(
      std::remove_if(
          findings.begin(),
          findings.end(),
          [this](const Finding& finding) {
            return finding.confidence < config_.minimum_confidence;
          }),
      findings.end());

  std::stable_sort(
      findings.begin(),
      findings.end(),
      [](const Finding& left, const Finding& right) {
        if (left.frame_index != right.frame_index) {
          return left.frame_index < right.frame_index;
        }

        if (std::abs(left.confidence - right.confidence) > 0.0001) {
          return left.confidence > right.confidence;
        }

        const int left_severity = SeverityRank(left.severity);
        const int right_severity = SeverityRank(right.severity);
        if (left_severity != right_severity) {
          return left_severity > right_severity;
        }

        return left.id < right.id;
      });

  std::vector<Finding> primaries;

  for (auto& finding : findings) {
    FindingRole role = FindingRole::kPrimary;

    for (const auto& primary : primaries) {
      if (ConflictsWithPrimary(finding, primary)) {
        role = FindingRole::kSecondary;
        break;
      }
    }

    if (role == FindingRole::kPrimary) {
      primaries.push_back(finding);
    }

    result.findings.push_back(ResolvedFinding{
        std::move(finding),
        role,
        RoleNote(role),
    });
  }

  return result;
}

double ConfidenceResolver::Calibrate(const Finding& finding) const {
  double confidence = finding.confidence;

  if (!HasEvidence(finding)) {
    confidence -= 0.20;
  }

  if (!HasConfirmationGuidance(finding)) {
    confidence -= 0.15;
  }

  if (finding.suggested_next_steps.empty()) {
    confidence -= 0.10;
  }

  if (finding.time_window_end_us <= finding.time_window_start_us) {
    confidence -= 0.10;
  }

  if (finding.severity == Severity::kCritical && confidence < 0.50) {
    confidence = 0.50;
  }

  if (confidence < 0.0) {
    return 0.0;
  }

  if (confidence > 0.95) {
    return 0.95;
  }

  return confidence;
}

bool ConfidenceResolver::ConflictsWithPrimary(const Finding& candidate, const Finding& primary) const {
  if (candidate.frame_index != primary.frame_index) {
    return false;
  }

  const auto candidate_start = static_cast<double>(candidate.time_window_start_us);
  const auto primary_start = static_cast<double>(primary.time_window_start_us);
  const double start_distance = std::abs(candidate_start - primary_start);

  return start_distance <= config_.same_frame_conflict_window_us;
}

}  // namespace riva

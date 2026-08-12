#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace riva {

enum class Severity {
  kInfo = 0,
  kWarning,
  kCritical,
};

// Evidence classification taxonomy implementing the Riva claim discipline.
// Every piece of evidence must declare how it was produced so that Riva
// never silently converts correlation into causation.
enum class EEvidenceClassification {
  kObserved = 0,    // Measured directly from telemetry.
  kDerived,         // Calculated deterministically from observed data.
  kCorrelated,      // Multiple observations exhibit a relationship.
  kInferred,        // Probable explanation derived from evidence.
  kSuspected,       // Hypothesis requiring developer validation.
  kRecommended,     // Suggested engineering action.
};

struct Evidence {
  std::string label;
  std::string value;
  EEvidenceClassification classification{EEvidenceClassification::kObserved};
};

struct Finding {
  std::string id;
  std::string title;
  Severity severity{Severity::kInfo};
  double confidence{0.0};
  std::size_t frame_index{0};
  std::uint64_t time_window_start_us{0};
  std::uint64_t time_window_end_us{0};
  std::vector<Evidence> evidence;
  std::vector<std::string> suggested_next_steps;
  std::vector<std::string> how_to_confirm;
  std::vector<std::string> related_finding_ids;
  std::string affected_thread;
  std::string affected_system;
};

}  // namespace riva

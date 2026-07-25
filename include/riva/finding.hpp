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

struct Evidence {
  std::string label;
  std::string value;
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
};

}  // namespace riva

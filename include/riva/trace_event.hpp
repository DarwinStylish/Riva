#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace riva {

struct TraceMetadata {
  std::string key;
  std::string value;
};

struct TraceEvent {
  std::string name;
  std::string category;
  std::string thread_name;
  std::uint64_t start_time_us{0};
  std::uint64_t duration_us{0};
  std::vector<TraceMetadata> metadata;

  [[nodiscard]] std::uint64_t EndTimeUs() const noexcept {
    return start_time_us + duration_us;
  }
};

}  // namespace riva

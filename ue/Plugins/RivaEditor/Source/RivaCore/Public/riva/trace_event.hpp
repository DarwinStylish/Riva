#pragma once

#include <cstdint>
#include <limits>
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
    if (duration_us > std::numeric_limits<std::uint64_t>::max() - start_time_us) {
      return std::numeric_limits<std::uint64_t>::max();
    }
    return start_time_us + duration_us;
  }
};

}  // namespace riva

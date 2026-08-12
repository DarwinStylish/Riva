#pragma once

#include <cstdint>
#include <string>

namespace riva {

// Time-series counter model for memory, physics, and other domain signals.
// Counters capture point-in-time scalar measurements associated with a
// specific telemetry category and unit.
struct FTraceCounter {
  std::string name;
  std::string category;
  std::string unit;  // e.g. "bytes", "ms", "count"
  std::uint64_t timestamp_us{0};
  double value{0.0};
};

}  // namespace riva

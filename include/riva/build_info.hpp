#pragma once

#include <cstdint>
#include <string>

namespace riva {

// Build metadata for regression intelligence.
// Maps directly to the Riva vision "Build" canonical entity, enabling
// regression tracking across builds, branches, platforms, and engine versions.
struct FBuildInfo {
  std::string build_id;
  std::string version;
  std::string branch;
  std::string commit;
  std::string configuration;   // e.g. "Development", "Shipping", "Test"
  std::string platform;        // e.g. "Win64", "PS5", "XboxSeriesX", "Linux"
  std::string engine_version;  // e.g. "5.5.1"
  std::uint64_t timestamp{0};  // Unix epoch seconds
};

// Scenario metadata for scenario-based baselines and analysis.
// Captures the gameplay context under which a trace was recorded.
struct FScenarioInfo {
  std::string scenario_id;
  std::string name;
  std::string map_name;
  std::string gameplay_state;
  std::uint32_t player_count{0};
  std::uint32_t agent_count{0};
};

}  // namespace riva

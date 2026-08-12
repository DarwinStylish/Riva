#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace riva {

// Thread type classification for cross-thread correlation and thread
// starvation detection. Covers all major Unreal Engine thread roles.
enum class EThreadType {
  kGameThread = 0,
  kRenderThread,
  kRhiThread,
  kWorkerThread,
  kAudioThread,
  kLoadingThread,
  kNetworkThread,
  kCustom,
};

// First-class thread entity for the Riva canonical telemetry model.
// Enables cross-thread correlation, utilization tracking, and thread
// starvation detection as described in the Riva vision architecture.
struct FTraceThread {
  std::size_t id{0};
  std::string name;
  EThreadType type{EThreadType::kCustom};
  std::uint32_t core_affinity{0};
  double utilization{0.0};       // 0.0 - 1.0
  double total_active_us{0.0};
  double total_wait_us{0.0};
  std::vector<std::size_t> event_indices;  // Indices into Frame::events
};

}  // namespace riva

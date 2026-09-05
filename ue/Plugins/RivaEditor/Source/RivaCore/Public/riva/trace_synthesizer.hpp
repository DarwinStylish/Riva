#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "riva/build_info.hpp"
#include "riva/export.hpp"
#include "riva/normalized_trace.hpp"
#include "riva/status.hpp"

namespace riva {

// Pathology type for synthetic trace injection.
// Each type maps to a corresponding builtin signature ID and produces
// the correct marker event names for detection.
enum class EPathologyType {
  kNone = 0,
  kShaderCompile,
  kPsoMiss,
  kStreamingIo,
  kGarbageCollection,
  kRhiSync,
  kCpuGameThread,
  kCpuRenderThread,
  kGpuVarianceLumen,
};

// A single pathology injection specification.
// Describes what stall type to inject, at which frame, and how severe.
struct FPathologyInjection {
  EPathologyType type{EPathologyType::kNone};
  std::size_t frame_index{0};
  double spike_ms{50.0};
  std::string marker_name;  // Auto-populated from type if empty
};

// Configuration for the synthetic trace generator.
struct FTraceSynthesizerConfig {
  std::string source_name{"synthetic"};
  std::size_t frame_count{120};
  double baseline_frame_ms{16.0};
  double baseline_jitter_ms{0.5};
  double game_thread_ratio{0.625};
  double render_thread_ratio{0.50};
  double gpu_ratio{0.70};
  std::vector<FPathologyInjection> injections;
  FBuildInfo build_info;
  FScenarioInfo scenario_info;
};

// Result of synthetic trace generation, containing the trace and
// the ground truth pathology injections for verification.
struct FSynthesizedTrace {
  NormalizedTrace trace;
  std::vector<FPathologyInjection> ground_truth;
  Status status;

  // Explicit constructor since NormalizedTrace requires a source_name.
  explicit FSynthesizedTrace(std::string source_name) : trace(std::move(source_name)) {}
};

// Deterministic synthetic telemetry generator.
// Produces NormalizedTrace instances with configurable pathology
// injections and known ground truth. Foundation for the performance
// pathology library described in the Riva vision.
class RIVACORE_API FTraceSynthesizer {
 public:
  [[nodiscard]] static FSynthesizedTrace Generate(const FTraceSynthesizerConfig& InConfig);
};

}  // namespace riva

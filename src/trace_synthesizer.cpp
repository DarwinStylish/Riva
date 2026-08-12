#include "riva/trace_synthesizer.hpp"

#include <cmath>
#include <cstdint>
#include <utility>

namespace riva {
namespace {

// Deterministic jitter using a simple linear congruential generator.
// Seeded from frame index to ensure reproducibility.
[[nodiscard]] double DeterministicJitter(std::size_t frame_index, double amplitude) {
  // LCG parameters (Numerical Recipes)
  const auto seed = static_cast<std::uint64_t>(frame_index) * 6364136223846793005ULL + 1442695040888963407ULL;
  const double normalized = static_cast<double>(seed >> 33) / static_cast<double>(1ULL << 31);
  return (normalized - 0.5) * 2.0 * amplitude;
}

// Return the default marker event name for a given pathology type.
[[nodiscard]] std::string DefaultMarkerName(EPathologyType type) {
  switch (type) {
    case EPathologyType::kShaderCompile:
      return "ShaderCompileWorker blocked frame";
    case EPathologyType::kPsoMiss:
      return "PSO Pipeline State creation";
    case EPathologyType::kStreamingIo:
      return "Async Loading streaming IO request";
    case EPathologyType::kGarbageCollection:
      return "GarbageCollect mark sweep";
    case EPathologyType::kRhiSync:
      return "RHIWait for present sync";
    case EPathologyType::kCpuGameThread:
      return "Blueprint Tick Execution";
    case EPathologyType::kCpuRenderThread:
      return "Scene Render Setup";
    case EPathologyType::kGpuVarianceLumen:
      return "Lumen virtual shadow map update";
    case EPathologyType::kNone:
      return "";
  }
  return "";
}

// Return the thread name where the marker event would fire.
[[nodiscard]] std::string DefaultThreadName(EPathologyType type) {
  switch (type) {
    case EPathologyType::kShaderCompile:
    case EPathologyType::kPsoMiss:
    case EPathologyType::kGarbageCollection:
    case EPathologyType::kCpuGameThread:
    case EPathologyType::kStreamingIo:
      return "GameThread";
    case EPathologyType::kCpuRenderThread:
    case EPathologyType::kRhiSync:
      return "RenderThread";
    case EPathologyType::kGpuVarianceLumen:
      return "RenderThread";
    case EPathologyType::kNone:
      return "";
  }
  return "";
}

// Return the event category for a given pathology type.
[[nodiscard]] std::string DefaultCategory(EPathologyType type) {
  switch (type) {
    case EPathologyType::kShaderCompile: return "Shader";
    case EPathologyType::kPsoMiss: return "RHI";
    case EPathologyType::kStreamingIo: return "Loading";
    case EPathologyType::kGarbageCollection: return "GC";
    case EPathologyType::kRhiSync: return "RHI";
    case EPathologyType::kCpuGameThread: return "GameThread";
    case EPathologyType::kCpuRenderThread: return "RenderThread";
    case EPathologyType::kGpuVarianceLumen: return "GPU";
    case EPathologyType::kNone: return "";
  }
  return "";
}

}  // namespace

FSynthesizedTrace FTraceSynthesizer::Generate(const FTraceSynthesizerConfig& InConfig) {
  FSynthesizedTrace result(InConfig.source_name);

  // Set build and scenario metadata
  result.trace.SetBuildInfo(InConfig.build_info);
  result.trace.SetScenarioInfo(InConfig.scenario_info);

  // Build a set of injection frame indices for fast lookup
  std::vector<const FPathologyInjection*> injection_at_frame(InConfig.frame_count, nullptr);
  for (const auto& injection : InConfig.injections) {
    if (injection.frame_index < InConfig.frame_count && injection.type != EPathologyType::kNone) {
      injection_at_frame[injection.frame_index] = &injection;
    }
  }

  for (std::size_t i = 0; i < InConfig.frame_count; ++i) {
    const double jitter = DeterministicJitter(i, InConfig.baseline_jitter_ms);
    const auto* injection = injection_at_frame[i];

    Frame frame;
    frame.index = i;
    frame.start_time_us = static_cast<std::uint64_t>(
        i * static_cast<std::size_t>(InConfig.baseline_frame_ms * 1000.0));

    if (injection != nullptr) {
      // Spike frame
      frame.duration_ms = injection->spike_ms;
      frame.game_thread_ms = injection->spike_ms * InConfig.game_thread_ratio;
      frame.render_thread_ms = injection->spike_ms * InConfig.render_thread_ratio;
      frame.gpu_ms = injection->spike_ms * InConfig.gpu_ratio;

      // Create marker event
      TraceEvent event;
      event.name = injection->marker_name.empty()
                       ? DefaultMarkerName(injection->type)
                       : injection->marker_name;
      event.category = DefaultCategory(injection->type);
      event.thread_name = DefaultThreadName(injection->type);
      event.start_time_us = frame.start_time_us + 1000;
      event.duration_us = static_cast<std::uint64_t>(
          (injection->spike_ms - InConfig.baseline_frame_ms) * 1000.0);

      frame.events.push_back(std::move(event));

      // Record ground truth
      FPathologyInjection truth = *injection;
      if (truth.marker_name.empty()) {
        truth.marker_name = DefaultMarkerName(truth.type);
      }
      result.ground_truth.push_back(std::move(truth));
    } else {
      // Baseline frame
      frame.duration_ms = InConfig.baseline_frame_ms + jitter;
      frame.game_thread_ms = (InConfig.baseline_frame_ms + jitter) * InConfig.game_thread_ratio;
      frame.render_thread_ms = (InConfig.baseline_frame_ms + jitter) * InConfig.render_thread_ratio;
      frame.gpu_ms = (InConfig.baseline_frame_ms + jitter) * InConfig.gpu_ratio;
    }

    // RHI is always a fraction of render thread
    frame.rhi_thread_ms = frame.render_thread_ms * 0.4;

    (void)result.trace.AddFrame(std::move(frame));
  }

  return result;
}

}  // namespace riva

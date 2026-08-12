#include "riva/builtin_signatures.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "riva/signature_result.hpp"
#include "riva/signature_utils.hpp"

namespace riva {
namespace {

struct SignatureRule {
  SignatureId id;
  Severity severity;
  double base_confidence;
  std::vector<std::string> markers;
  std::vector<std::string> next_steps;
  std::vector<std::string> confirmation_steps;
  std::string affected_system;
};

class MarkerSignature final : public ISignature {
 public:
  explicit MarkerSignature(SignatureRule rule)
      : rule_(std::move(rule)) {}

  [[nodiscard]] std::string id() const override {
    return ToStableString(rule_.id);
  }

  [[nodiscard]] std::string name() const override {
    return ToDisplayName(rule_.id);
  }

  [[nodiscard]] std::vector<Finding> Analyze(
      const NormalizedTrace& trace,
      const std::vector<Spike>& spikes) const override {
    std::vector<Finding> findings;

    for (const auto& spike : spikes) {
      const SpikeContext context = BuildSpikeContext(trace, spike);
      if (context.frame == nullptr) {
        continue;
      }

      const auto events = EventsInWindow(*context.frame, context.window_start_us, context.window_end_us);

      std::vector<Evidence> evidence;
      std::string detected_thread;
      for (const TraceEvent* event : events) {
        if (event != nullptr && EventMatchesAny(*event, rule_.markers)) {
          evidence.push_back(Evidence{"event", event->name, EEvidenceClassification::kObserved});
          if (detected_thread.empty() && !event->thread_name.empty()) {
            detected_thread = event->thread_name;
          }
        }
      }

      if (evidence.empty()) {
        continue;
      }

      evidence.push_back(Evidence{"frame_ms", FormatMilliseconds(spike.frame_ms), EEvidenceClassification::kObserved});
      evidence.push_back(Evidence{"baseline_ms", FormatMilliseconds(spike.baseline_ms), EEvidenceClassification::kObserved});
      evidence.push_back(Evidence{"delta_ms", FormatMilliseconds(spike.delta_ms), EEvidenceClassification::kDerived});

      Finding finding;
      finding.id = id();
      finding.title = name();
      finding.severity = rule_.severity;
      finding.confidence = ClampConfidence(rule_.base_confidence + static_cast<double>(evidence.size()) * 0.03);
      finding.frame_index = spike.frame_index;
      finding.time_window_start_us = context.window_start_us;
      finding.time_window_end_us = context.window_end_us;
      finding.evidence = evidence;
      finding.suggested_next_steps = rule_.next_steps;
      finding.how_to_confirm = rule_.confirmation_steps;
      finding.affected_thread = detected_thread;
      finding.affected_system = rule_.affected_system;
      findings.push_back(std::move(finding));
    }

    return findings;
  }

 private:
  SignatureRule rule_;
};

class CpuThreadSignature final : public ISignature {
 public:
  CpuThreadSignature(SignatureId id, Severity severity, std::string metric_name)
      : id_(id), severity_(severity), metric_name_(std::move(metric_name)) {}

  [[nodiscard]] std::string id() const override {
    return ToStableString(id_);
  }

  [[nodiscard]] std::string name() const override {
    return ToDisplayName(id_);
  }

  [[nodiscard]] std::vector<Finding> Analyze(
      const NormalizedTrace& trace,
      const std::vector<Spike>& spikes) const override {
    std::vector<Finding> findings;

    for (const auto& spike : spikes) {
      const SpikeContext context = BuildSpikeContext(trace, spike);
      if (context.frame == nullptr) {
        continue;
      }

      const double thread_ms =
          id_ == SignatureId::kCpuGameThread ? context.frame->game_thread_ms
                                             : context.frame->render_thread_ms;

      if (thread_ms < spike.baseline_ms * 0.75 && thread_ms < 20.0) {
        continue;
      }

      Finding finding;
      finding.id = id();
      finding.title = name();
      finding.severity = severity_;
      finding.confidence = ClampConfidence(0.62 + (thread_ms / spike.frame_ms) * 0.25);
      finding.frame_index = spike.frame_index;
      finding.time_window_start_us = context.window_start_us;
      finding.time_window_end_us = context.window_end_us;
      finding.evidence = {
          Evidence{"frame_ms", FormatMilliseconds(spike.frame_ms), EEvidenceClassification::kObserved},
          Evidence{metric_name_, FormatMilliseconds(thread_ms), EEvidenceClassification::kObserved},
          Evidence{"baseline_ms", FormatMilliseconds(spike.baseline_ms), EEvidenceClassification::kObserved},
      };
      finding.suggested_next_steps = {
          "Inspect the CPU timing lane for the affected frame.",
          "Sort timers by inclusive time in Unreal Insights.",
          "Compare the spike frame against nearby non-spike frames.",
      };
      finding.how_to_confirm = {
          "Open the frame in Unreal Insights and verify the dominant CPU timer.",
          "Check whether the same thread dominates the total frame duration.",
      };
      finding.affected_thread = id_ == SignatureId::kCpuGameThread ? "GameThread" : "RenderThread";
      finding.affected_system = "CPU";
      findings.push_back(std::move(finding));
    }

    return findings;
  }

 private:
  SignatureId id_;
  Severity severity_;
  std::string metric_name_;
};

class GpuVarianceSignature final : public ISignature {
 public:
  [[nodiscard]] std::string id() const override {
    return ToStableString(SignatureId::kGpuVarianceLumenVsm);
  }

  [[nodiscard]] std::string name() const override {
    return ToDisplayName(SignatureId::kGpuVarianceLumenVsm);
  }

  [[nodiscard]] std::vector<Finding> Analyze(
      const NormalizedTrace& trace,
      const std::vector<Spike>& spikes) const override {
    std::vector<Finding> findings;

    for (const auto& spike : spikes) {
      const SpikeContext context = BuildSpikeContext(trace, spike);
      if (context.frame == nullptr || context.frame->gpu_ms < 20.0) {
        continue;
      }

      const auto events = EventsInWindow(*context.frame, context.window_start_us, context.window_end_us);

      bool has_gpu_marker = false;
      for (const TraceEvent* event : events) {
        if (event != nullptr &&
            EventMatchesAny(*event, {"lumen", "virtual shadow", "vsm", "shadow depth"})) {
          has_gpu_marker = true;
          break;
        }
      }

      if (!has_gpu_marker && context.frame->gpu_ms < spike.frame_ms * 0.60) {
        continue;
      }

      Finding finding;
      finding.id = id();
      finding.title = name();
      finding.severity = Severity::kWarning;
      finding.confidence = ClampConfidence(has_gpu_marker ? 0.74 : 0.58);
      finding.frame_index = spike.frame_index;
      finding.time_window_start_us = context.window_start_us;
      finding.time_window_end_us = context.window_end_us;
      finding.evidence = {
          Evidence{"frame_ms", FormatMilliseconds(spike.frame_ms), EEvidenceClassification::kObserved},
          Evidence{"gpu_ms", FormatMilliseconds(context.frame->gpu_ms), EEvidenceClassification::kObserved},
          Evidence{"baseline_ms", FormatMilliseconds(spike.baseline_ms), EEvidenceClassification::kObserved},
      };
      finding.suggested_next_steps = {
          "Inspect the GPU track in Unreal Insights.",
          "Check Lumen, virtual shadow map, and shadow depth passes.",
          "Compare GPU pass timings against nearby stable frames.",
      };
      finding.how_to_confirm = {
          "Confirm the spike is GPU-bound in Unreal Insights.",
          "Verify whether Lumen or virtual shadow map passes expand in the spike window.",
      };
      finding.affected_thread = "GPU";
      finding.affected_system = "Rendering";
      findings.push_back(std::move(finding));
    }

    return findings;
  }
};

std::unique_ptr<ISignature> MakeMarkerSignature(SignatureRule rule) {
  return std::make_unique<MarkerSignature>(std::move(rule));
}

}  // namespace

std::vector<std::unique_ptr<ISignature>> CreateBuiltinSignatures() {
  std::vector<std::unique_ptr<ISignature>> signatures;

  signatures.push_back(MakeMarkerSignature(SignatureRule{
      SignatureId::kShaderCompile,
      Severity::kCritical,
      0.78,
      {"shadercompile", "shader compile", "compile shader"},
      {
          "Check whether shaders were compiling during gameplay or editor interaction.",
          "Warm relevant shaders before the measured scenario.",
          "Review material or permutation changes near the spike.",
      },
      {
          "Open the spike window in Unreal Insights and inspect shader compilation markers.",
          "Check whether shader worker activity overlaps the frame hitch.",
      },
      "Rendering",
  }));

  signatures.push_back(MakeMarkerSignature(SignatureRule{
      SignatureId::kPsoMiss,
      Severity::kCritical,
      0.76,
      {"pso", "pipeline state", "pipeline"},
      {
          "Review PSO precaching coverage for the affected content.",
          "Check whether a new material, mesh, or render state appeared in the spike window.",
          "Validate PSO cache generation for the target platform.",
      },
      {
          "Open the RHI and rendering tracks in Unreal Insights.",
          "Look for PSO creation or pipeline state compilation overlapping the hitch.",
      },
      "Rendering",
  }));

  signatures.push_back(MakeMarkerSignature(SignatureRule{
      SignatureId::kStreamingIo,
      Severity::kWarning,
      0.70,
      {"streaming", "async loading", "io dispatcher", "pak", "zen"},
      {
          "Inspect async loading and IO activity around the spike.",
          "Check asset residency and streaming budgets.",
          "Review level streaming or world partition activity near the hitch.",
      },
      {
          "Open loading, file IO, or streaming markers in Unreal Insights.",
          "Confirm whether asset loading overlaps the spike time window.",
      },
      "Loading",
  }));

  signatures.push_back(std::make_unique<CpuThreadSignature>(
      SignatureId::kCpuGameThread,
      Severity::kWarning,
      "game_thread_ms"));

  signatures.push_back(std::make_unique<CpuThreadSignature>(
      SignatureId::kCpuRenderThread,
      Severity::kWarning,
      "render_thread_ms"));

  signatures.push_back(MakeMarkerSignature(SignatureRule{
      SignatureId::kRhiSync,
      Severity::kWarning,
      0.68,
      {"rhiwait", "rhi wait", "wait for rhi", "present", "sync"},
      {
          "Inspect RHI wait markers and render thread synchronization.",
          "Check whether CPU is waiting on GPU or presentation.",
          "Compare RHI waits against GPU timing in the same window.",
      },
      {
          "Open RHI thread markers in Unreal Insights.",
          "Confirm whether synchronization waits overlap the spike frame.",
      },
      "Rendering",
  }));

  signatures.push_back(MakeMarkerSignature(SignatureRule{
      SignatureId::kGarbageCollection,
      Severity::kCritical,
      0.80,
      {"garbagecollect", "garbage collection", "gc mark", "gc sweep", "collect garbage"},
      {
          "Inspect UObject allocation and GC cadence.",
          "Avoid triggering expensive GC during active gameplay.",
          "Review object lifetime churn near the spike.",
      },
      {
          "Open GC markers in Unreal Insights.",
          "Confirm mark, sweep, or purge work overlaps the spike window.",
      },
      "Memory",
  }));

  signatures.push_back(std::make_unique<GpuVarianceSignature>());

  return signatures;
}

}  // namespace riva

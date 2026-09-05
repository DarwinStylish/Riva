# Authoring Custom Diagnostic Signatures

Riva's diagnostic pipeline is extensible. Developers can write custom signatures to detect game-specific performance stalls (e.g. Niagara particle system compilation, Chaos physics bottlenecks, or World Partitioning cell loading).


## The `ISignature` Interface

All signatures implement the `ISignature` interface defined in `riva/signature.hpp`:

```cpp
namespace riva {

class ISignature {
 public:
  virtual ~ISignature() = default;

  [[nodiscard]] virtual std::string id() const = 0;
  [[nodiscard]] virtual std::string name() const = 0;

  [[nodiscard]] virtual std::vector<Finding> Analyze(
      const NormalizedTrace& trace,
      const std::vector<Spike>& spikes) const = 0;
};

}  // namespace riva
```


## Example: Custom Physics Spike Signature

Below is a complete example of a custom signature detecting physics stalls:

```cpp
#include "riva/signature.hpp"
#include "riva/signature_utils.hpp"

namespace riva {

class PhysicsSpikeSignature final : public ISignature {
 public:
  [[nodiscard]] std::string id() const override {
    return "STUT_PHYSICS_CHAOS";
  }

  [[nodiscard]] std::string name() const override {
    return "Physics step solver stall";
  }

  [[nodiscard]] std::vector<Finding> Analyze(
      const NormalizedTrace& trace,
      const std::vector<Spike>& spikes) const override {
    std::vector<Finding> findings;

    for (const auto& spike : spikes) {
      const SpikeContext context = BuildSpikeContext(trace, spike);
      if (context.frame == nullptr) continue;

      const auto events = EventsInWindow(*context.frame, context.window_start_us, context.window_end_us);

      bool found_physics_marker = false;
      for (const TraceEvent* event : events) {
        if (event != nullptr && event->name.find("ChaosPhysicsStep") != std::string::npos) {
          found_physics_marker = true;
          break;
        }
      }

      if (!found_physics_marker) continue;

      Finding finding;
      finding.id = id();
      finding.title = name();
      finding.severity = Severity::kWarning;
      finding.confidence = 0.85;
      finding.frame_index = spike.frame_index;
      finding.time_window_start_us = context.window_start_us;
      finding.time_window_end_us = context.window_end_us;

      finding.affected_thread = "GameThread";
      finding.affected_system = "Physics";

      finding.evidence.push_back(Evidence{"event", "ChaosPhysicsStep", EEvidenceClassification::kObserved});
      finding.evidence.push_back(Evidence{"frame_ms", FormatMilliseconds(spike.frame_ms), EEvidenceClassification::kObserved});

      finding.suggested_next_steps.push_back("Check active rigid body count in the physics scene.");
      finding.how_to_confirm.push_back("Inspect Chaos physics step timing track in Unreal Insights.");

      findings.push_back(std::move(finding));
    }

    return findings;
  }
};

}  // namespace riva
```


## Registering Custom Signatures

To register a custom signature with the `AnalysisEngine`:

```cpp
std::vector<std::unique_ptr<riva::ISignature>> signatures = riva::CreateBuiltinSignatures();

// Add your custom signature
signatures.push_back(std::make_unique<riva::PhysicsSpikeSignature>());

// Initialize engine with expanded signatures
riva::AnalysisEngine engine(std::move(signatures));
riva::AnalysisResult result = engine.Analyze(trace);
```

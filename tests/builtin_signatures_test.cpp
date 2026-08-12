#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "riva/builtin_signatures.hpp"
#include "riva/normalized_trace.hpp"
#include "riva/signature_result.hpp"
#include "riva/spike_detector.hpp"

namespace {

void Expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << "\n";
    std::exit(1);
  }
}

riva::TraceEvent MakeEvent(std::string name, std::uint64_t start_us, std::uint64_t duration_us) {
  riva::TraceEvent event;
  event.name = std::move(name);
  event.category = "Test";
  event.thread_name = "GameThread";
  event.start_time_us = start_us;
  event.duration_us = duration_us;
  return event;
}

riva::NormalizedTrace MakeTraceWithSpikeEvent(std::string event_name) {
  riva::NormalizedTrace trace("signature-test");

  for (std::size_t i = 0; i < 6; ++i) {
    riva::Frame frame;
    frame.index = i;
    frame.start_time_us = static_cast<std::uint64_t>(i * 16000);
    frame.duration_ms = i == 5 ? 50.0 : 16.0;
    frame.game_thread_ms = i == 5 ? 44.0 : 10.0;
    frame.render_thread_ms = i == 5 ? 42.0 : 9.0;
    frame.rhi_thread_ms = i == 5 ? 30.0 : 5.0;
    frame.gpu_ms = i == 5 ? 38.0 : 12.0;

    if (i == 5) {
      frame.events.push_back(MakeEvent(event_name, frame.start_time_us + 1000, 5000));
    }

    auto status = trace.AddFrame(std::move(frame));
    Expect(status.ok(), "test frame should be accepted");
  }

  return trace;
}

std::vector<riva::Spike> DetectSpikes(const riva::NormalizedTrace& trace) {
  riva::RollingMedianSpikeDetector detector;
  auto spikes = detector.Detect(trace);
  Expect(spikes.size() == 1, "test trace must produce one spike");
  return spikes;
}

bool HasFinding(const std::vector<std::unique_ptr<riva::ISignature>>& signatures,
                const riva::NormalizedTrace& trace,
                const std::vector<riva::Spike>& spikes,
                const std::string& id) {
  for (const auto& signature : signatures) {
    if (signature->id() != id) {
      continue;
    }

    const auto findings = signature->Analyze(trace, spikes);
    if (!findings.empty()) {
      Expect(!findings[0].how_to_confirm.empty(), "finding must include confirmation guidance");
      Expect(!findings[0].suggested_next_steps.empty(), "finding must include next steps");
      Expect(!findings[0].affected_system.empty(), "finding must include affected_system");
      for (const auto& ev : findings[0].evidence) {
        // Verify classification is not left at an invalid state; it should be kObserved or kDerived
        Expect(ev.classification == riva::EEvidenceClassification::kObserved ||
               ev.classification == riva::EEvidenceClassification::kDerived,
               "evidence classification must be OBSERVED or DERIVED for builtin signatures");
      }
      return true;
    }
  }

  return false;
}

void TestSignatureIds() {
  Expect(riva::ToStableString(riva::SignatureId::kShaderCompile) == "STUT_SHADER_COMPILE",
         "shader compile id must be stable");
  Expect(riva::ToStableString(riva::SignatureId::kPsoMiss) == "STUT_PSO_MISS",
         "pso id must be stable");
  Expect(riva::ToStableString(riva::SignatureId::kStreamingIo) == "STUT_STREAMING_IO",
         "streaming id must be stable");
  Expect(riva::ToStableString(riva::SignatureId::kCpuGameThread) == "STUT_CPU_GT",
         "game thread id must be stable");
  Expect(riva::ToStableString(riva::SignatureId::kCpuRenderThread) == "STUT_CPU_RT",
         "render thread id must be stable");
  Expect(riva::ToStableString(riva::SignatureId::kRhiSync) == "STUT_RHI_SYNC",
         "rhi id must be stable");
  Expect(riva::ToStableString(riva::SignatureId::kGarbageCollection) == "STUT_GC",
         "gc id must be stable");
  Expect(riva::ToStableString(riva::SignatureId::kGpuVarianceLumenVsm) ==
             "STUT_GPU_VARIANCE_LUMEN_VSM",
         "gpu variance id must be stable");
}

void TestBuiltinSignatureCount() {
  const auto signatures = riva::CreateBuiltinSignatures();
  Expect(signatures.size() == 8, "PR3 must register eight builtin signatures");
}

void TestMarkerDrivenSignatures() {
  const auto signatures = riva::CreateBuiltinSignatures();

  {
    auto trace = MakeTraceWithSpikeEvent("ShaderCompileWorker blocked frame");
    auto spikes = DetectSpikes(trace);
    Expect(HasFinding(signatures, trace, spikes, "STUT_SHADER_COMPILE"),
           "shader compile marker must produce finding");
  }

  {
    auto trace = MakeTraceWithSpikeEvent("PSO Pipeline State creation");
    auto spikes = DetectSpikes(trace);
    Expect(HasFinding(signatures, trace, spikes, "STUT_PSO_MISS"),
           "pso marker must produce finding");
  }

  {
    auto trace = MakeTraceWithSpikeEvent("Async Loading streaming IO request");
    auto spikes = DetectSpikes(trace);
    Expect(HasFinding(signatures, trace, spikes, "STUT_STREAMING_IO"),
           "streaming marker must produce finding");
  }

  {
    auto trace = MakeTraceWithSpikeEvent("RHIWait for present sync");
    auto spikes = DetectSpikes(trace);
    Expect(HasFinding(signatures, trace, spikes, "STUT_RHI_SYNC"),
           "rhi wait marker must produce finding");
  }

  {
    auto trace = MakeTraceWithSpikeEvent("GarbageCollect mark sweep");
    auto spikes = DetectSpikes(trace);
    Expect(HasFinding(signatures, trace, spikes, "STUT_GC"),
           "gc marker must produce finding");
  }

  {
    auto trace = MakeTraceWithSpikeEvent("Lumen virtual shadow map update");
    auto spikes = DetectSpikes(trace);
    Expect(HasFinding(signatures, trace, spikes, "STUT_GPU_VARIANCE_LUMEN_VSM"),
           "gpu variance marker must produce finding");
  }
}

void TestCpuThreadSignatures() {
  const auto signatures = riva::CreateBuiltinSignatures();
  auto trace = MakeTraceWithSpikeEvent("generic gameplay spike");
  auto spikes = DetectSpikes(trace);

  Expect(HasFinding(signatures, trace, spikes, "STUT_CPU_GT"),
         "game thread timing must produce finding");
  Expect(HasFinding(signatures, trace, spikes, "STUT_CPU_RT"),
         "render thread timing must produce finding");
}

void TestAffectedFieldsAndEvidenceClassification() {
  const auto signatures = riva::CreateBuiltinSignatures();

  // Test marker signature (shader compile) sets affected_thread from event
  {
    auto trace = MakeTraceWithSpikeEvent("ShaderCompileWorker blocked frame");
    auto spikes = DetectSpikes(trace);
    for (const auto& sig : signatures) {
      if (sig->id() != "STUT_SHADER_COMPILE") continue;
      const auto findings = sig->Analyze(trace, spikes);
      Expect(!findings.empty(), "shader compile must produce finding");
      Expect(findings[0].affected_thread == "GameThread",
             "shader compile affected_thread must come from event");
      Expect(findings[0].affected_system == "Rendering",
             "shader compile affected_system must be Rendering");
      // Verify delta_ms is classified as kDerived
      bool found_derived = false;
      for (const auto& ev : findings[0].evidence) {
        if (ev.label == "delta_ms") {
          Expect(ev.classification == riva::EEvidenceClassification::kDerived,
                 "delta_ms must be classified as DERIVED");
          found_derived = true;
        }
      }
      Expect(found_derived, "delta_ms evidence must exist");
      break;
    }
  }

  // Test CPU thread signature sets affected_thread directly
  {
    auto trace = MakeTraceWithSpikeEvent("generic gameplay spike");
    auto spikes = DetectSpikes(trace);
    for (const auto& sig : signatures) {
      if (sig->id() != "STUT_CPU_GT") continue;
      const auto findings = sig->Analyze(trace, spikes);
      Expect(!findings.empty(), "game thread must produce finding");
      Expect(findings[0].affected_thread == "GameThread",
             "game thread finding must set affected_thread");
      Expect(findings[0].affected_system == "CPU",
             "game thread finding affected_system must be CPU");
      break;
    }
  }
}

}  // namespace

int main() {
  TestSignatureIds();
  TestBuiltinSignatureCount();
  TestMarkerDrivenSignatures();
  TestCpuThreadSignatures();
  TestAffectedFieldsAndEvidenceClassification();
  return 0;
}

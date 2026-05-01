#include <cstdlib>
#include <iostream>

#include "riva/json_trace_loader.hpp"

namespace {

void Expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << "\n";
    std::exit(1);
  }
}

void TestLoadsValidTrace() {
  const auto result = riva::LoadNormalizedTraceFromJsonText(R"JSON({
    "source_name": "unit-json",
    "frames": [
      {
        "index": 0,
        "start_time_us": 0,
        "duration_ms": 16.0,
        "game_thread_ms": 10.0,
        "render_thread_ms": 8.0,
        "gpu_ms": 11.0,
        "events": []
      },
      {
        "index": 1,
        "start_time_us": 16000,
        "duration_ms": 48.0,
        "game_thread_ms": 40.0,
        "render_thread_ms": 12.0,
        "gpu_ms": 14.0,
        "events": [
          {
            "name": "ShaderCompileWorker blocked frame",
            "category": "Shader",
            "thread_name": "GameThread",
            "start_time_us": 17000,
            "duration_us": 4000,
            "metadata": [
              {
                "key": "asset",
                "value": "M_Test"
              }
            ]
          }
        ]
      }
    ]
  })JSON");

  Expect(result.status.ok(), "valid JSON trace should load");
  Expect(result.trace.has_value(), "loaded result should contain trace");
  Expect(result.trace->source_name() == "unit-json", "source name should load");
  Expect(result.trace->frame_count() == 2, "two frames should load");
  Expect(result.trace->frames()[1].events.size() == 1, "event should load");
  Expect(result.trace->frames()[1].events[0].metadata.size() == 1, "metadata should load");
}

void TestRejectsMalformedJson() {
  const auto result = riva::LoadNormalizedTraceFromJsonText("{");
  Expect(!result.status.ok(), "malformed JSON should fail");
  Expect(result.status.code() == riva::StatusCode::kParseError, "malformed JSON should be parse error");
}

void TestRejectsMissingRequiredField() {
  const auto result = riva::LoadNormalizedTraceFromJsonText(R"JSON({
    "source_name": "missing-frames"
  })JSON");

  Expect(!result.status.ok(), "missing frames should fail");
}

void TestRejectsUnknownFieldByDefault() {
  const auto result = riva::LoadNormalizedTraceFromJsonText(R"JSON({
    "source_name": "unknown-field",
    "frames": [],
    "extra": true
  })JSON");

  Expect(!result.status.ok(), "unknown root field should fail by default");
}

void TestAllowsUnknownFieldWhenConfigured() {
  const auto result = riva::LoadNormalizedTraceFromJsonText(R"JSON({
    "source_name": "unknown-field",
    "frames": [],
    "extra": true
  })JSON", riva::JsonTraceLoaderOptions{true});

  Expect(result.status.ok(), "unknown field should be allowed when configured");
}

void TestRejectsNonIncreasingFrames() {
  const auto result = riva::LoadNormalizedTraceFromJsonText(R"JSON({
    "source_name": "bad-index",
    "frames": [
      {
        "index": 1,
        "start_time_us": 0,
        "duration_ms": 16.0
      },
      {
        "index": 1,
        "start_time_us": 16000,
        "duration_ms": 18.0
      }
    ]
  })JSON");

  Expect(!result.status.ok(), "duplicate frame index should fail");
}

}  // namespace

int main() {
  TestLoadsValidTrace();
  TestRejectsMalformedJson();
  TestRejectsMissingRequiredField();
  TestRejectsUnknownFieldByDefault();
  TestAllowsUnknownFieldWhenConfigured();
  TestRejectsNonIncreasingFrames();
  return 0;
}

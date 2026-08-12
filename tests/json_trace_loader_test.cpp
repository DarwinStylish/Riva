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

void TestLoadsExpandedFrameFields() {
  const auto result = riva::LoadNormalizedTraceFromJsonText(R"JSON({
    "source_name": "expanded-json",
    "frames": [
      {
        "index": 0,
        "start_time_us": 0,
        "duration_ms": 16.0,
        "game_thread_ms": 10.0,
        "render_thread_ms": 8.0,
        "gpu_ms": 11.0,
        "physics_ms": 2.5,
        "ai_ms": 1.3,
        "network_ms": 0.8,
        "loading_ms": 0.4,
        "memory_bytes": 536870912.0,
        "events": [],
        "counters": [
          {
            "name": "PhysicsObjects",
            "category": "Physics",
            "unit": "count",
            "timestamp_us": 500,
            "value": 42.0
          }
        ]
      }
    ]
  })JSON");

  Expect(result.status.ok(), "expanded JSON trace should load");
  Expect(result.trace.has_value(), "result should contain trace");

  const auto& frame = result.trace->frames()[0];
  Expect(frame.physics_ms == 2.5, "physics_ms should parse");
  Expect(frame.ai_ms == 1.3, "ai_ms should parse");
  Expect(frame.network_ms == 0.8, "network_ms should parse");
  Expect(frame.loading_ms == 0.4, "loading_ms should parse");
  Expect(frame.memory_bytes == 536870912.0, "memory_bytes should parse");
  Expect(frame.counters.size() == 1, "counters should parse");
  Expect(frame.counters[0].name == "PhysicsObjects", "counter name should parse");
  Expect(frame.counters[0].unit == "count", "counter unit should parse");
  Expect(frame.counters[0].value == 42.0, "counter value should parse");
}

void TestLoadsBuildInfoAndScenarioInfo() {
  const auto result = riva::LoadNormalizedTraceFromJsonText(R"JSON({
    "source_name": "build-json",
    "build_info": {
      "build_id": "B-2847",
      "version": "0.2.0",
      "branch": "feat/nanite",
      "commit": "a1b2c3d",
      "configuration": "Development",
      "platform": "Win64",
      "engine_version": "5.5.1",
      "timestamp": 1722096000
    },
    "scenario_info": {
      "scenario_id": "forest-flythrough",
      "name": "Forest Flythrough Benchmark",
      "map_name": "ForestBenchmark_P",
      "gameplay_state": "gameplay",
      "player_count": 1,
      "agent_count": 50
    },
    "frames": [
      {
        "index": 0,
        "start_time_us": 0,
        "duration_ms": 16.0
      }
    ]
  })JSON");

  Expect(result.status.ok(), "build/scenario JSON should load");
  Expect(result.trace.has_value(), "result should contain trace");

  const auto& build = result.trace->build_info();
  Expect(build.build_id == "B-2847", "build_id should parse");
  Expect(build.branch == "feat/nanite", "branch should parse");
  Expect(build.platform == "Win64", "platform should parse");
  Expect(build.engine_version == "5.5.1", "engine_version should parse");
  Expect(build.timestamp == 1722096000, "timestamp should parse");

  const auto& scenario = result.trace->scenario_info();
  Expect(scenario.scenario_id == "forest-flythrough", "scenario_id should parse");
  Expect(scenario.name == "Forest Flythrough Benchmark", "scenario name should parse");
  Expect(scenario.map_name == "ForestBenchmark_P", "map_name should parse");
  Expect(scenario.player_count == 1, "player_count should parse");
  Expect(scenario.agent_count == 50, "agent_count should parse");
}

void TestLoadsThreads() {
  const auto result = riva::LoadNormalizedTraceFromJsonText(R"JSON({
    "source_name": "thread-json",
    "threads": [
      {
        "id": 0,
        "name": "GameThread",
        "type": "GameThread",
        "utilization": 0.85,
        "total_active_us": 14000.0,
        "total_wait_us": 2000.0
      },
      {
        "id": 1,
        "name": "RenderThread",
        "type": "RenderThread",
        "utilization": 0.72
      }
    ],
    "frames": [
      {
        "index": 0,
        "start_time_us": 0,
        "duration_ms": 16.0
      }
    ]
  })JSON");

  Expect(result.status.ok(), "thread JSON should load");
  Expect(result.trace.has_value(), "result should contain trace");
  Expect(result.trace->threads().size() == 2, "two threads should parse");
  Expect(result.trace->threads()[0].name == "GameThread", "thread name should parse");
  Expect(result.trace->threads()[0].type == riva::EThreadType::kGameThread, "thread type should parse");
  Expect(result.trace->threads()[1].name == "RenderThread", "second thread name should parse");
}

int main() {
  TestLoadsValidTrace();
  TestRejectsMalformedJson();
  TestRejectsMissingRequiredField();
  TestRejectsUnknownFieldByDefault();
  TestAllowsUnknownFieldWhenConfigured();
  TestRejectsNonIncreasingFrames();
  TestLoadsExpandedFrameFields();
  TestLoadsBuildInfoAndScenarioInfo();
  TestLoadsThreads();
  return 0;
}

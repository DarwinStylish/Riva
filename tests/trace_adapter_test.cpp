#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "riva/default_analysis.hpp"
#include "riva/json_trace_adapter.hpp"
#include "riva/trace_adapter_registry.hpp"

namespace {

void Expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << "\n";
    std::exit(1);
  }
}

std::filesystem::path WriteTempJsonTrace() {
  const auto path = std::filesystem::temp_directory_path() / "riva_adapter_test_trace.json";

  std::ofstream file(path);
  file << R"JSON({
    "source_name": "adapter-test",
    "frames": [
      {
        "index": 0,
        "start_time_us": 0,
        "duration_ms": 16.0,
        "game_thread_ms": 9.0,
        "render_thread_ms": 7.0,
        "gpu_ms": 10.0,
        "events": []
      },
      {
        "index": 1,
        "start_time_us": 16000,
        "duration_ms": 16.2,
        "game_thread_ms": 9.2,
        "render_thread_ms": 7.1,
        "gpu_ms": 10.3,
        "events": []
      },
      {
        "index": 2,
        "start_time_us": 32000,
        "duration_ms": 48.0,
        "game_thread_ms": 42.0,
        "render_thread_ms": 12.0,
        "gpu_ms": 13.0,
        "events": [
          {
            "name": "ShaderCompileWorker blocked frame",
            "category": "Shader",
            "thread_name": "GameThread",
            "start_time_us": 34000,
            "duration_us": 6000
          }
        ]
      }
    ]
  })JSON";

  return path;
}

void TestJsonAdapterSupportsJsonPaths() {
  const riva::JsonTraceAdapter adapter;

  Expect(adapter.name() == "json", "json adapter name should be stable");
  Expect(adapter.SupportsPath("trace.json"), "json adapter should support .json");
  Expect(adapter.SupportsPath("TRACE.JSON"), "json adapter should support uppercase .JSON");
  Expect(!adapter.SupportsPath("trace.utrace"), "json adapter should not support .utrace");
}

void TestJsonAdapterLoadsTrace() {
  const auto path = WriteTempJsonTrace();

  const riva::JsonTraceAdapter adapter;
  const auto result = adapter.Load(path.string());

  Expect(result.status.ok(), "json adapter should load valid trace");
  Expect(result.trace.has_value(), "json adapter result should contain trace");
  Expect(result.trace->source_name() == "adapter-test", "trace source name should load");
  Expect(result.trace->frame_count() == 3, "trace should contain three frames");

  std::filesystem::remove(path);
}

void TestDefaultRegistryFindsJsonAdapter() {
  auto registry = riva::CreateDefaultTraceAdapterRegistry();

  Expect(registry.size() == 1, "default registry should register one adapter");
  Expect(registry.FindForPath("trace.json") != nullptr, "default registry should find json adapter");
  Expect(registry.FindForPath("trace.utrace") == nullptr, "default registry should not claim utrace yet");
}

void TestRegistryRejectsUnsupportedPath() {
  auto registry = riva::CreateDefaultTraceAdapterRegistry();
  const auto result = registry.Load("trace.utrace");

  Expect(!result.status.ok(), "unsupported path should fail");
  Expect(result.status.code() == riva::StatusCode::kInvalidArgument,
         "unsupported path should be invalid argument");
}

void TestDefaultAnalysisEngineRunsLoadedTrace() {
  const auto path = WriteTempJsonTrace();

  auto registry = riva::CreateDefaultTraceAdapterRegistry();
  auto load_result = registry.Load(path.string());

  Expect(load_result.status.ok(), "registry should load valid json trace");
  Expect(load_result.trace.has_value(), "registry load should contain trace");

  auto engine = riva::CreateDefaultAnalysisEngine();
  const auto analysis = engine.Analyze(*load_result.trace);

  Expect(!analysis.findings.empty(), "default analysis engine should produce findings");

  std::filesystem::remove(path);
}

}  // namespace

int main() {
  TestJsonAdapterSupportsJsonPaths();
  TestJsonAdapterLoadsTrace();
  TestDefaultRegistryFindsJsonAdapter();
  TestRegistryRejectsUnsupportedPath();
  TestDefaultAnalysisEngineRunsLoadedTrace();
  return 0;
}

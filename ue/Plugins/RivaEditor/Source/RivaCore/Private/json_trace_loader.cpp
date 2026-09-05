#include "riva/json_trace_loader.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "riva/json_utils.hpp"

namespace riva {
namespace {

[[nodiscard]] Status ParseMetadata(const JsonValue& metadata_value,
                                   std::vector<TraceMetadata>& metadata,
                                   const JsonTraceLoaderOptions& options) {
  if (metadata_value.type != JsonValue::Type::kArray) {
    return Status(StatusCode::kInvalidArgument, "metadata must be an array");
  }

  for (const auto& item : metadata_value.array_value) {
    if (item.type != JsonValue::Type::kObject) {
      return Status(StatusCode::kInvalidArgument, "metadata item must be an object");
    }

    auto status = StrictKeys(item, {"key", "value"}, "metadata", options.allow_unknown_fields);
    if (!status.ok()) {
      return status;
    }

    const JsonValue* key = FindField(item, "key");
    const JsonValue* value = FindField(item, "value");

    status = RequireType(key, JsonValue::Type::kString, "metadata.key");
    if (!status.ok()) {
      return status;
    }

    status = RequireType(value, JsonValue::Type::kString, "metadata.value");
    if (!status.ok()) {
      return status;
    }

    metadata.push_back(TraceMetadata{key->string_value, value->string_value});
  }

  return Status::Ok();
}

[[nodiscard]] Status ParseEvent(const JsonValue& event_value, TraceEvent& event,
                                const JsonTraceLoaderOptions& options) {
  if (event_value.type != JsonValue::Type::kObject) {
    return Status(StatusCode::kInvalidArgument, "event must be an object");
  }

  auto status = StrictKeys(
      event_value, {"name", "category", "thread_name", "start_time_us", "duration_us", "metadata"},
      "event", options.allow_unknown_fields);
  if (!status.ok()) {
    return status;
  }

  const JsonValue* name = FindField(event_value, "name");
  const JsonValue* start = FindField(event_value, "start_time_us");
  const JsonValue* duration = FindField(event_value, "duration_us");

  status = RequireType(name, JsonValue::Type::kString, "event.name");
  if (!status.ok()) {
    return status;
  }

  status = RequireType(start, JsonValue::Type::kNumber, "event.start_time_us");
  if (!status.ok()) {
    return status;
  }

  status = RequireType(duration, JsonValue::Type::kNumber, "event.duration_us");
  if (!status.ok()) {
    return status;
  }

  event.name = name->string_value;

  status = NumberToUint64(start->number_value, event.start_time_us, "event.start_time_us");
  if (!status.ok()) {
    return status;
  }

  status = NumberToUint64(duration->number_value, event.duration_us, "event.duration_us");
  if (!status.ok()) {
    return status;
  }

  status = ReadOptionalString(event_value, "category", event.category);
  if (!status.ok()) {
    return status;
  }

  status = ReadOptionalString(event_value, "thread_name", event.thread_name);
  if (!status.ok()) {
    return status;
  }

  const JsonValue* metadata = FindField(event_value, "metadata");
  if (metadata != nullptr) {
    status = ParseMetadata(*metadata, event.metadata, options);
    if (!status.ok()) {
      return status;
    }
  }

  return Status::Ok();
}

[[nodiscard]] Status ParseCounter(const JsonValue& counter_value, FTraceCounter& counter,
                                  const JsonTraceLoaderOptions& options) {
  if (counter_value.type != JsonValue::Type::kObject) {
    return Status(StatusCode::kInvalidArgument, "counter must be an object");
  }

  auto status = StrictKeys(counter_value, {"name", "category", "unit", "timestamp_us", "value"},
                           "counter", options.allow_unknown_fields);
  if (!status.ok()) {
    return status;
  }

  const JsonValue* name = FindField(counter_value, "name");
  const JsonValue* value = FindField(counter_value, "value");

  status = RequireType(name, JsonValue::Type::kString, "counter.name");
  if (!status.ok()) {
    return status;
  }

  status = RequireType(value, JsonValue::Type::kNumber, "counter.value");
  if (!status.ok()) {
    return status;
  }

  counter.name = name->string_value;
  counter.value = value->number_value;

  status = ReadOptionalString(counter_value, "category", counter.category);
  if (!status.ok()) {
    return status;
  }

  status = ReadOptionalString(counter_value, "unit", counter.unit);
  if (!status.ok()) {
    return status;
  }

  const JsonValue* timestamp = FindField(counter_value, "timestamp_us");
  if (timestamp != nullptr) {
    status = RequireType(timestamp, JsonValue::Type::kNumber, "counter.timestamp_us");
    if (!status.ok()) {
      return status;
    }
    status = NumberToUint64(timestamp->number_value, counter.timestamp_us, "counter.timestamp_us");
    if (!status.ok()) {
      return status;
    }
  }

  return Status::Ok();
}

[[nodiscard]] Status ParseFrame(const JsonValue& frame_value, Frame& frame,
                                const JsonTraceLoaderOptions& options) {
  if (frame_value.type != JsonValue::Type::kObject) {
    return Status(StatusCode::kInvalidArgument, "frame must be an object");
  }

  auto status = StrictKeys(frame_value,
                           {"index", "start_time_us", "duration_ms", "game_thread_ms",
                            "render_thread_ms", "rhi_thread_ms", "gpu_ms", "physics_ms", "ai_ms",
                            "network_ms", "loading_ms", "memory_bytes", "events", "counters"},
                           "frame", options.allow_unknown_fields);
  if (!status.ok()) {
    return status;
  }

  const JsonValue* index = FindField(frame_value, "index");
  const JsonValue* start = FindField(frame_value, "start_time_us");
  const JsonValue* duration = FindField(frame_value, "duration_ms");

  status = RequireType(index, JsonValue::Type::kNumber, "frame.index");
  if (!status.ok()) {
    return status;
  }

  status = RequireType(start, JsonValue::Type::kNumber, "frame.start_time_us");
  if (!status.ok()) {
    return status;
  }

  status = RequireType(duration, JsonValue::Type::kNumber, "frame.duration_ms");
  if (!status.ok()) {
    return status;
  }

  status = NumberToSize(index->number_value, frame.index, "frame.index");
  if (!status.ok()) {
    return status;
  }

  status = NumberToUint64(start->number_value, frame.start_time_us, "frame.start_time_us");
  if (!status.ok()) {
    return status;
  }

  frame.duration_ms = duration->number_value;

  status = ReadOptionalNumber(frame_value, "game_thread_ms", frame.game_thread_ms);
  if (!status.ok()) {
    return status;
  }

  status = ReadOptionalNumber(frame_value, "render_thread_ms", frame.render_thread_ms);
  if (!status.ok()) {
    return status;
  }

  status = ReadOptionalNumber(frame_value, "rhi_thread_ms", frame.rhi_thread_ms);
  if (!status.ok()) {
    return status;
  }

  status = ReadOptionalNumber(frame_value, "gpu_ms", frame.gpu_ms);
  if (!status.ok()) {
    return status;
  }

  status = ReadOptionalNumber(frame_value, "physics_ms", frame.physics_ms);
  if (!status.ok()) {
    return status;
  }

  status = ReadOptionalNumber(frame_value, "ai_ms", frame.ai_ms);
  if (!status.ok()) {
    return status;
  }

  status = ReadOptionalNumber(frame_value, "network_ms", frame.network_ms);
  if (!status.ok()) {
    return status;
  }

  status = ReadOptionalNumber(frame_value, "loading_ms", frame.loading_ms);
  if (!status.ok()) {
    return status;
  }

  status = ReadOptionalNumber(frame_value, "memory_bytes", frame.memory_bytes);
  if (!status.ok()) {
    return status;
  }

  // Parse events
  const JsonValue* events = FindField(frame_value, "events");
  if (events != nullptr) {
    if (events->type != JsonValue::Type::kArray) {
      return Status(StatusCode::kInvalidArgument, "frame.events must be an array");
    }

    for (const auto& event_value : events->array_value) {
      TraceEvent event;
      status = ParseEvent(event_value, event, options);
      if (!status.ok()) {
        return status;
      }

      frame.events.push_back(std::move(event));
    }
  }

  // Parse counters
  const JsonValue* counters = FindField(frame_value, "counters");
  if (counters != nullptr) {
    if (counters->type != JsonValue::Type::kArray) {
      return Status(StatusCode::kInvalidArgument, "frame.counters must be an array");
    }

    for (const auto& counter_value : counters->array_value) {
      FTraceCounter counter;
      status = ParseCounter(counter_value, counter, options);
      if (!status.ok()) {
        return status;
      }

      frame.counters.push_back(std::move(counter));
    }
  }

  return Status::Ok();
}

[[nodiscard]] Status ParseBuildInfo(const JsonValue& build_value, FBuildInfo& build_info,
                                    const JsonTraceLoaderOptions& options) {
  if (build_value.type != JsonValue::Type::kObject) {
    return Status(StatusCode::kInvalidArgument, "build_info must be an object");
  }

  auto status = StrictKeys(build_value,
                           {"build_id", "version", "branch", "commit", "configuration", "platform",
                            "engine_version", "timestamp"},
                           "build_info", options.allow_unknown_fields);
  if (!status.ok()) {
    return status;
  }

  status = ReadOptionalString(build_value, "build_id", build_info.build_id);
  if (!status.ok()) return status;

  status = ReadOptionalString(build_value, "version", build_info.version);
  if (!status.ok()) return status;

  status = ReadOptionalString(build_value, "branch", build_info.branch);
  if (!status.ok()) return status;

  status = ReadOptionalString(build_value, "commit", build_info.commit);
  if (!status.ok()) return status;

  status = ReadOptionalString(build_value, "configuration", build_info.configuration);
  if (!status.ok()) return status;

  status = ReadOptionalString(build_value, "platform", build_info.platform);
  if (!status.ok()) return status;

  status = ReadOptionalString(build_value, "engine_version", build_info.engine_version);
  if (!status.ok()) return status;

  const JsonValue* timestamp = FindField(build_value, "timestamp");
  if (timestamp != nullptr) {
    status = RequireType(timestamp, JsonValue::Type::kNumber, "build_info.timestamp");
    if (!status.ok()) return status;
    status = NumberToUint64(timestamp->number_value, build_info.timestamp, "build_info.timestamp");
    if (!status.ok()) return status;
  }

  return Status::Ok();
}

[[nodiscard]] Status ParseScenarioInfo(const JsonValue& scenario_value,
                                       FScenarioInfo& scenario_info,
                                       const JsonTraceLoaderOptions& options) {
  if (scenario_value.type != JsonValue::Type::kObject) {
    return Status(StatusCode::kInvalidArgument, "scenario_info must be an object");
  }

  auto status = StrictKeys(
      scenario_value,
      {"scenario_id", "name", "map_name", "gameplay_state", "player_count", "agent_count"},
      "scenario_info", options.allow_unknown_fields);
  if (!status.ok()) {
    return status;
  }

  status = ReadOptionalString(scenario_value, "scenario_id", scenario_info.scenario_id);
  if (!status.ok()) return status;

  status = ReadOptionalString(scenario_value, "name", scenario_info.name);
  if (!status.ok()) return status;

  status = ReadOptionalString(scenario_value, "map_name", scenario_info.map_name);
  if (!status.ok()) return status;

  status = ReadOptionalString(scenario_value, "gameplay_state", scenario_info.gameplay_state);
  if (!status.ok()) return status;

  const JsonValue* player_count = FindField(scenario_value, "player_count");
  if (player_count != nullptr) {
    status = RequireType(player_count, JsonValue::Type::kNumber, "scenario_info.player_count");
    if (!status.ok()) return status;
    std::size_t count = 0;
    status = NumberToSize(player_count->number_value, count, "scenario_info.player_count");
    if (!status.ok()) return status;
    if (count > std::numeric_limits<std::uint32_t>::max()) {
      return Status(StatusCode::kInvalidArgument,
                    "field exceeds uint32 range: scenario_info.player_count");
    }
    scenario_info.player_count = static_cast<std::uint32_t>(count);
  }

  const JsonValue* agent_count = FindField(scenario_value, "agent_count");
  if (agent_count != nullptr) {
    status = RequireType(agent_count, JsonValue::Type::kNumber, "scenario_info.agent_count");
    if (!status.ok()) return status;
    std::size_t count = 0;
    status = NumberToSize(agent_count->number_value, count, "scenario_info.agent_count");
    if (!status.ok()) return status;
    if (count > std::numeric_limits<std::uint32_t>::max()) {
      return Status(StatusCode::kInvalidArgument,
                    "field exceeds uint32 range: scenario_info.agent_count");
    }
    scenario_info.agent_count = static_cast<std::uint32_t>(count);
  }

  return Status::Ok();
}

[[nodiscard]] Status ParseThread(const JsonValue& thread_value, FTraceThread& thread,
                                 const JsonTraceLoaderOptions& options) {
  if (thread_value.type != JsonValue::Type::kObject) {
    return Status(StatusCode::kInvalidArgument, "thread must be an object");
  }

  auto status = StrictKeys(
      thread_value,
      {"id", "name", "type", "core_affinity", "utilization", "total_active_us", "total_wait_us"},
      "thread", options.allow_unknown_fields);
  if (!status.ok()) {
    return status;
  }

  const JsonValue* id = FindField(thread_value, "id");
  status = RequireType(id, JsonValue::Type::kNumber, "thread.id");
  if (!status.ok()) return status;
  status = NumberToSize(id->number_value, thread.id, "thread.id");
  if (!status.ok()) return status;

  const JsonValue* name = FindField(thread_value, "name");
  status = RequireType(name, JsonValue::Type::kString, "thread.name");
  if (!status.ok()) return status;
  thread.name = name->string_value;

  // Parse optional thread type string
  std::string type_str;
  status = ReadOptionalString(thread_value, "type", type_str);
  if (!status.ok()) return status;

  if (type_str == "GameThread") {
    thread.type = EThreadType::kGameThread;
  } else if (type_str == "RenderThread") {
    thread.type = EThreadType::kRenderThread;
  } else if (type_str == "RhiThread") {
    thread.type = EThreadType::kRhiThread;
  } else if (type_str == "WorkerThread") {
    thread.type = EThreadType::kWorkerThread;
  } else if (type_str == "AudioThread") {
    thread.type = EThreadType::kAudioThread;
  } else if (type_str == "LoadingThread") {
    thread.type = EThreadType::kLoadingThread;
  } else if (type_str == "NetworkThread") {
    thread.type = EThreadType::kNetworkThread;
  } else {
    thread.type = EThreadType::kCustom;
  }

  double utilization = 0.0;
  status = ReadOptionalNumber(thread_value, "utilization", utilization);
  if (!status.ok()) return status;
  thread.utilization = utilization;

  status = ReadOptionalNumber(thread_value, "total_active_us", thread.total_active_us);
  if (!status.ok()) return status;

  status = ReadOptionalNumber(thread_value, "total_wait_us", thread.total_wait_us);
  if (!status.ok()) return status;

  const JsonValue* core_affinity = FindField(thread_value, "core_affinity");
  if (core_affinity != nullptr) {
    status = RequireType(core_affinity, JsonValue::Type::kNumber, "thread.core_affinity");
    if (!status.ok()) return status;
    std::size_t affinity = 0;
    status = NumberToSize(core_affinity->number_value, affinity, "thread.core_affinity");
    if (!status.ok()) return status;
    if (affinity > std::numeric_limits<std::uint32_t>::max()) {
      return Status(StatusCode::kInvalidArgument,
                    "field exceeds uint32 range: thread.core_affinity");
    }
    thread.core_affinity = static_cast<std::uint32_t>(affinity);
  }

  return Status::Ok();
}

}  // namespace

JsonTraceLoadResult LoadNormalizedTraceFromJsonText(std::string_view json_text,
                                                    const JsonTraceLoaderOptions& options) {
  JsonValue root;
  JsonParser parser(json_text, options.max_input_bytes, options.max_nesting_depth);

  if (!parser.Parse(root)) {
    return JsonTraceLoadResult{
        Status(StatusCode::kParseError, parser.error()),
        std::nullopt,
    };
  }

  if (root.type != JsonValue::Type::kObject) {
    return JsonTraceLoadResult{
        Status(StatusCode::kInvalidArgument, "root must be an object"),
        std::nullopt,
    };
  }

  auto status =
      StrictKeys(root, {"source_name", "frames", "build_info", "scenario_info", "threads"}, "root",
                 options.allow_unknown_fields);
  if (!status.ok()) {
    return JsonTraceLoadResult{status, std::nullopt};
  }

  const JsonValue* source_name = FindField(root, "source_name");
  const JsonValue* frames = FindField(root, "frames");

  status = RequireType(source_name, JsonValue::Type::kString, "source_name");
  if (!status.ok()) {
    return JsonTraceLoadResult{status, std::nullopt};
  }

  status = RequireType(frames, JsonValue::Type::kArray, "frames");
  if (!status.ok()) {
    return JsonTraceLoadResult{status, std::nullopt};
  }

  NormalizedTrace trace(source_name->string_value);

  // Parse optional build_info
  const JsonValue* build_info = FindField(root, "build_info");
  if (build_info != nullptr) {
    FBuildInfo info;
    status = ParseBuildInfo(*build_info, info, options);
    if (!status.ok()) {
      return JsonTraceLoadResult{status, std::nullopt};
    }
    trace.SetBuildInfo(std::move(info));
  }

  // Parse optional scenario_info
  const JsonValue* scenario_info = FindField(root, "scenario_info");
  if (scenario_info != nullptr) {
    FScenarioInfo info;
    status = ParseScenarioInfo(*scenario_info, info, options);
    if (!status.ok()) {
      return JsonTraceLoadResult{status, std::nullopt};
    }
    trace.SetScenarioInfo(std::move(info));
  }

  // Parse optional threads
  const JsonValue* threads = FindField(root, "threads");
  if (threads != nullptr) {
    if (threads->type != JsonValue::Type::kArray) {
      return JsonTraceLoadResult{Status(StatusCode::kInvalidArgument, "threads must be an array"),
                                 std::nullopt};
    }
    for (const auto& thread_value : threads->array_value) {
      FTraceThread thread;
      status = ParseThread(thread_value, thread, options);
      if (!status.ok()) {
        return JsonTraceLoadResult{status, std::nullopt};
      }
      status = trace.AddThread(std::move(thread));
      if (!status.ok()) {
        return JsonTraceLoadResult{status, std::nullopt};
      }
    }
  }

  // Parse frames
  for (const auto& frame_value : frames->array_value) {
    Frame frame;
    status = ParseFrame(frame_value, frame, options);
    if (!status.ok()) {
      return JsonTraceLoadResult{status, std::nullopt};
    }

    status = trace.AddFrame(std::move(frame));
    if (!status.ok()) {
      return JsonTraceLoadResult{status, std::nullopt};
    }
  }

  return JsonTraceLoadResult{Status::Ok(), std::move(trace)};
}

JsonTraceLoadResult LoadNormalizedTraceFromJsonFile(const std::string& path,
                                                    const JsonTraceLoaderOptions& options) {
  std::ifstream file(path);
  if (!file) {
    return JsonTraceLoadResult{
        Status(StatusCode::kNotFound, "could not open JSON trace file: " + path),
        std::nullopt,
    };
  }

  file.seekg(0, std::ios::end);
  const std::streampos FileSize = file.tellg();
  if (FileSize < 0) {
    return JsonTraceLoadResult{
        Status(StatusCode::kInternalError, "could not determine JSON trace file size: " + path),
        std::nullopt,
    };
  }
  if (static_cast<std::uintmax_t>(static_cast<std::streamoff>(FileSize)) >
      options.max_input_bytes) {
    return JsonTraceLoadResult{
        Status(StatusCode::kInvalidArgument, "JSON trace file exceeds configured size limit"),
        std::nullopt,
    };
  }
  file.seekg(0, std::ios::beg);

  std::ostringstream buffer;
  buffer << file.rdbuf();
  if (file.bad()) {
    return JsonTraceLoadResult{
        Status(StatusCode::kInternalError, "could not read JSON trace file: " + path),
        std::nullopt,
    };
  }

  return LoadNormalizedTraceFromJsonText(buffer.str(), options);
}

}  // namespace riva

#include "riva/json_trace_loader.hpp"
#include "riva/json_utils.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace riva {
namespace {

[[nodiscard]] Status ParseMetadata(
    const JsonValue& metadata_value,
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

[[nodiscard]] Status ParseEvent(
    const JsonValue& event_value,
    TraceEvent& event,
    const JsonTraceLoaderOptions& options) {
  if (event_value.type != JsonValue::Type::kObject) {
    return Status(StatusCode::kInvalidArgument, "event must be an object");
  }

  auto status = StrictKeys(
      event_value,
      {"name", "category", "thread_name", "start_time_us", "duration_us", "metadata"},
      "event",
      options.allow_unknown_fields);
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

[[nodiscard]] Status ParseFrame(
    const JsonValue& frame_value,
    Frame& frame,
    const JsonTraceLoaderOptions& options) {
  if (frame_value.type != JsonValue::Type::kObject) {
    return Status(StatusCode::kInvalidArgument, "frame must be an object");
  }

  auto status = StrictKeys(
      frame_value,
      {"index", "start_time_us", "duration_ms", "game_thread_ms", "render_thread_ms",
       "rhi_thread_ms", "gpu_ms", "events"},
      "frame",
      options.allow_unknown_fields);
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

  const JsonValue* events = FindField(frame_value, "events");
  if (events == nullptr) {
    return Status::Ok();
  }

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

  return Status::Ok();
}

}  // namespace

JsonTraceLoadResult LoadNormalizedTraceFromJsonText(
    std::string_view json_text,
    JsonTraceLoaderOptions options) {
  JsonValue root;
  JsonParser parser(json_text);

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

  auto status = StrictKeys(root, {"source_name", "frames"}, "root", options.allow_unknown_fields);
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

JsonTraceLoadResult LoadNormalizedTraceFromJsonFile(
    const std::string& path,
    JsonTraceLoaderOptions options) {
  std::ifstream file(path);
  if (!file) {
    return JsonTraceLoadResult{
        Status(StatusCode::kNotFound, "could not open JSON trace file: " + path),
        std::nullopt,
    };
  }

  std::ostringstream buffer;
  buffer << file.rdbuf();

  return LoadNormalizedTraceFromJsonText(buffer.str(), options);
}

}  // namespace riva

#include "riva/json_trace_loader.hpp"

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

struct JsonValue {
  enum class Type {
    kNull,
    kBool,
    kNumber,
    kString,
    kArray,
    kObject,
  };

  Type type{Type::kNull};
  bool bool_value{false};
  double number_value{0.0};
  std::string string_value;
  std::vector<JsonValue> array_value;
  std::map<std::string, JsonValue> object_value;
};

class JsonParser {
 public:
  explicit JsonParser(std::string_view input) : input_(input) {}

  [[nodiscard]] bool Parse(JsonValue& value) {
    SkipWhitespace();

    if (!ParseValue(value)) {
      return false;
    }

    SkipWhitespace();

    if (position_ != input_.size()) {
      return Fail("unexpected trailing content");
    }

    return true;
  }

  [[nodiscard]] const std::string& error() const noexcept {
    return error_;
  }

 private:
  [[nodiscard]] bool ParseValue(JsonValue& value) {
    SkipWhitespace();

    if (AtEnd()) {
      return Fail("unexpected end of JSON");
    }

    const char c = Peek();

    if (c == '{') {
      return ParseObject(value);
    }

    if (c == '[') {
      return ParseArray(value);
    }

    if (c == '"') {
      value.type = JsonValue::Type::kString;
      return ParseString(value.string_value);
    }

    if (c == '-' || IsDigit(c)) {
      value.type = JsonValue::Type::kNumber;
      return ParseNumber(value.number_value);
    }

    if (StartsWith("true")) {
      position_ += 4;
      value.type = JsonValue::Type::kBool;
      value.bool_value = true;
      return true;
    }

    if (StartsWith("false")) {
      position_ += 5;
      value.type = JsonValue::Type::kBool;
      value.bool_value = false;
      return true;
    }

    if (StartsWith("null")) {
      position_ += 4;
      value.type = JsonValue::Type::kNull;
      return true;
    }

    return Fail("invalid JSON value");
  }

  [[nodiscard]] bool ParseObject(JsonValue& value) {
    value.type = JsonValue::Type::kObject;
    (void)Consume('{');
    SkipWhitespace();

    if (!AtEnd() && Peek() == '}') {
      (void)Consume('}');
      return true;
    }

    while (!AtEnd()) {
      std::string key;
      if (!ParseString(key)) {
        return false;
      }

      SkipWhitespace();
      if (!Consume(':')) {
        return Fail("expected ':' after object key");
      }

      JsonValue child;
      if (!ParseValue(child)) {
        return false;
      }

      if (value.object_value.find(key) != value.object_value.end()) {
        return Fail("duplicate object key: " + key);
      }

      value.object_value.emplace(std::move(key), std::move(child));
      SkipWhitespace();

      if (Consume('}')) {
        return true;
      }

      if (!Consume(',')) {
        return Fail("expected ',' or '}' in object");
      }

      SkipWhitespace();
    }

    return Fail("unterminated object");
  }

  [[nodiscard]] bool ParseArray(JsonValue& value) {
    value.type = JsonValue::Type::kArray;
    (void)Consume('[');
    SkipWhitespace();

    if (!AtEnd() && Peek() == ']') {
      (void)Consume(']');
      return true;
    }

    while (!AtEnd()) {
      JsonValue child;
      if (!ParseValue(child)) {
        return false;
      }

      value.array_value.push_back(std::move(child));
      SkipWhitespace();

      if (Consume(']')) {
        return true;
      }

      if (!Consume(',')) {
        return Fail("expected ',' or ']' in array");
      }

      SkipWhitespace();
    }

    return Fail("unterminated array");
  }

  [[nodiscard]] bool ParseString(std::string& out) {
    if (!Consume('"')) {
      return Fail("expected string");
    }

    while (!AtEnd()) {
      const char c = Advance();

      if (c == '"') {
        return true;
      }

      if (c == '\\') {
        if (AtEnd()) {
          return Fail("unterminated string escape");
        }

        const char escaped = Advance();
        switch (escaped) {
          case '"':
          case '\\':
          case '/':
            out.push_back(escaped);
            break;
          case 'b':
            out.push_back('\b');
            break;
          case 'f':
            out.push_back('\f');
            break;
          case 'n':
            out.push_back('\n');
            break;
          case 'r':
            out.push_back('\r');
            break;
          case 't':
            out.push_back('\t');
            break;
          default:
            return Fail("unsupported string escape");
        }

        continue;
      }

      if (static_cast<unsigned char>(c) < 0x20) {
        return Fail("control character inside string");
      }

      out.push_back(c);
    }

    return Fail("unterminated string");
  }

  [[nodiscard]] bool ParseNumber(double& out) {
    const std::size_t start = position_;

    if (Peek() == '-') {
      (void)Advance();
    }

    if (AtEnd()) {
      return Fail("invalid number");
    }

    if (Peek() == '0') {
      (void)Advance();
    } else if (IsDigit(Peek())) {
      while (!AtEnd() && IsDigit(Peek())) {
        (void)Advance();
      }
    } else {
      return Fail("invalid number");
    }

    if (!AtEnd() && Peek() == '.') {
      (void)Advance();

      if (AtEnd() || !IsDigit(Peek())) {
        return Fail("invalid number fraction");
      }

      while (!AtEnd() && IsDigit(Peek())) {
        (void)Advance();
      }
    }

    if (!AtEnd() && (Peek() == 'e' || Peek() == 'E')) {
      (void)Advance();

      if (!AtEnd() && (Peek() == '+' || Peek() == '-')) {
        (void)Advance();
      }

      if (AtEnd() || !IsDigit(Peek())) {
        return Fail("invalid number exponent");
      }

      while (!AtEnd() && IsDigit(Peek())) {
        (void)Advance();
      }
    }

    try {
      out = std::stod(std::string(input_.substr(start, position_ - start)));
    } catch (...) {
      return Fail("number conversion failed");
    }

    if (!std::isfinite(out)) {
      return Fail("number must be finite");
    }

    return true;
  }

  void SkipWhitespace() {
    while (!AtEnd()) {
      const char c = Peek();
      if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
        (void)Advance();
        continue;
      }

      break;
    }
  }

  [[nodiscard]] bool StartsWith(std::string_view token) const {
    return input_.substr(position_, token.size()) == token;
  }

  [[nodiscard]] bool Consume(char expected) {
    if (!AtEnd() && Peek() == expected) {
      ++position_;
      return true;
    }

    return false;
  }

  [[nodiscard]] char Advance() {
    return input_[position_++];
  }

  [[nodiscard]] char Peek() const {
    return input_[position_];
  }

  [[nodiscard]] bool AtEnd() const {
    return position_ >= input_.size();
  }

  [[nodiscard]] static bool IsDigit(char c) {
    return c >= '0' && c <= '9';
  }

  [[nodiscard]] bool Fail(std::string message) {
    error_ = std::move(message);
    return false;
  }

  std::string_view input_;
  std::size_t position_{0};
  std::string error_;
};

[[nodiscard]] Status StrictKeys(
    const JsonValue& value,
    const std::set<std::string>& allowed,
    const std::string& object_name,
    bool allow_unknown_fields) {
  if (allow_unknown_fields) {
    return Status::Ok();
  }

  for (const auto& [key, unused] : value.object_value) {
    (void)unused;
    if (allowed.find(key) == allowed.end()) {
      return Status(StatusCode::kInvalidArgument, "unknown key in " + object_name + ": " + key);
    }
  }

  return Status::Ok();
}

[[nodiscard]] const JsonValue* FindField(const JsonValue& object, const std::string& key) {
  const auto found = object.object_value.find(key);
  if (found == object.object_value.end()) {
    return nullptr;
  }

  return &found->second;
}

[[nodiscard]] Status RequireType(const JsonValue* value, JsonValue::Type type, const std::string& name) {
  if (value == nullptr) {
    return Status(StatusCode::kInvalidArgument, "missing required field: " + name);
  }

  if (value->type != type) {
    return Status(StatusCode::kInvalidArgument, "invalid type for field: " + name);
  }

  return Status::Ok();
}

[[nodiscard]] Status NumberToUint64(double value, std::uint64_t& out, const std::string& name) {
  if (value < 0.0 || std::floor(value) != value) {
    return Status(StatusCode::kInvalidArgument, "field must be a non-negative integer: " + name);
  }

  out = static_cast<std::uint64_t>(value);
  return Status::Ok();
}

[[nodiscard]] Status NumberToSize(double value, std::size_t& out, const std::string& name) {
  if (value < 0.0 || std::floor(value) != value) {
    return Status(StatusCode::kInvalidArgument, "field must be a non-negative integer: " + name);
  }

  out = static_cast<std::size_t>(value);
  return Status::Ok();
}

[[nodiscard]] Status ReadOptionalString(
    const JsonValue& object,
    const std::string& key,
    std::string& out) {
  const JsonValue* value = FindField(object, key);
  if (value == nullptr) {
    return Status::Ok();
  }

  if (value->type != JsonValue::Type::kString) {
    return Status(StatusCode::kInvalidArgument, "invalid type for field: " + key);
  }

  out = value->string_value;
  return Status::Ok();
}

[[nodiscard]] Status ReadOptionalNumber(
    const JsonValue& object,
    const std::string& key,
    double& out) {
  const JsonValue* value = FindField(object, key);
  if (value == nullptr) {
    return Status::Ok();
  }

  if (value->type != JsonValue::Type::kNumber) {
    return Status(StatusCode::kInvalidArgument, "invalid type for field: " + key);
  }

  out = value->number_value;
  return Status::Ok();
}

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

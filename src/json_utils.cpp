#include "riva/json_utils.hpp"
#include <cmath>

namespace riva {

JsonParser::JsonParser(std::string_view input) : input_(input) {}

bool JsonParser::Parse(JsonValue& value) {
  SkipWhitespace();
  if (!ParseValue(value)) return false;
  SkipWhitespace();
  if (position_ != input_.size()) return Fail("unexpected trailing content");
  return true;
}

bool JsonParser::ParseValue(JsonValue& value) {
  SkipWhitespace();
  if (AtEnd()) return Fail("unexpected end of JSON");

  const char c = Peek();
  if (c == '{') return ParseObject(value);
  if (c == '[') return ParseArray(value);
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

bool JsonParser::ParseObject(JsonValue& value) {
  value.type = JsonValue::Type::kObject;
  (void)Consume('{');
  SkipWhitespace();

  if (!AtEnd() && Peek() == '}') {
    (void)Consume('}');
    return true;
  }

  while (!AtEnd()) {
    std::string key;
    if (!ParseString(key)) return false;
    SkipWhitespace();
    if (!Consume(':')) return Fail("expected ':' after object key");

    JsonValue child;
    if (!ParseValue(child)) return false;

    if (value.object_value.find(key) != value.object_value.end()) {
      return Fail("duplicate object key: " + key);
    }
    value.object_value.emplace(std::move(key), std::move(child));
    SkipWhitespace();

    if (Consume('}')) return true;
    if (!Consume(',')) return Fail("expected ',' or '}' in object");
    SkipWhitespace();
  }
  return Fail("unterminated object");
}

bool JsonParser::ParseArray(JsonValue& value) {
  value.type = JsonValue::Type::kArray;
  (void)Consume('[');
  SkipWhitespace();

  if (!AtEnd() && Peek() == ']') {
    (void)Consume(']');
    return true;
  }

  while (!AtEnd()) {
    JsonValue child;
    if (!ParseValue(child)) return false;
    value.array_value.push_back(std::move(child));
    SkipWhitespace();

    if (Consume(']')) return true;
    if (!Consume(',')) return Fail("expected ',' or ']' in array");
    SkipWhitespace();
  }
  return Fail("unterminated array");
}

bool JsonParser::ParseString(std::string& out) {
  if (!Consume('"')) return Fail("expected string");
  while (!AtEnd()) {
    const char c = Advance();
    if (c == '"') return true;
    if (c == '\\') {
      if (AtEnd()) return Fail("unterminated string escape");
      const char escaped = Advance();
      switch (escaped) {
        case '"': case '\\': case '/': out.push_back(escaped); break;
        case 'b': out.push_back('\b'); break;
        case 'f': out.push_back('\f'); break;
        case 'n': out.push_back('\n'); break;
        case 'r': out.push_back('\r'); break;
        case 't': out.push_back('\t'); break;
        default: return Fail("unsupported string escape");
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

bool JsonParser::ParseNumber(double& out) {
  const std::size_t start = position_;
  if (Peek() == '-') (void)Advance();
  if (AtEnd()) return Fail("invalid number");

  if (Peek() == '0') {
    (void)Advance();
  } else if (IsDigit(Peek())) {
    while (!AtEnd() && IsDigit(Peek())) (void)Advance();
  } else {
    return Fail("invalid number");
  }

  if (!AtEnd() && Peek() == '.') {
    (void)Advance();
    if (AtEnd() || !IsDigit(Peek())) return Fail("invalid number fraction");
    while (!AtEnd() && IsDigit(Peek())) (void)Advance();
  }

  if (!AtEnd() && (Peek() == 'e' || Peek() == 'E')) {
    (void)Advance();
    if (!AtEnd() && (Peek() == '+' || Peek() == '-')) (void)Advance();
    if (AtEnd() || !IsDigit(Peek())) return Fail("invalid number exponent");
    while (!AtEnd() && IsDigit(Peek())) (void)Advance();
  }

  try {
    out = std::stod(std::string(input_.substr(start, position_ - start)));
  } catch (...) {
    return Fail("number conversion failed");
  }
  if (!std::isfinite(out)) return Fail("number must be finite");
  return true;
}

void JsonParser::SkipWhitespace() {
  while (!AtEnd()) {
    const char c = Peek();
    if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
      (void)Advance();
      continue;
    }
    break;
  }
}

bool JsonParser::StartsWith(std::string_view token) const {
  return input_.substr(position_, token.size()) == token;
}

bool JsonParser::Consume(char expected) {
  if (!AtEnd() && Peek() == expected) {
    ++position_;
    return true;
  }
  return false;
}

char JsonParser::Advance() { return input_[position_++]; }
char JsonParser::Peek() const { return input_[position_]; }
bool JsonParser::AtEnd() const { return position_ >= input_.size(); }
bool JsonParser::IsDigit(char c) { return c >= '0' && c <= '9'; }
bool JsonParser::Fail(std::string message) {
  error_ = std::move(message);
  return false;
}

Status StrictKeys(
    const JsonValue& value,
    const std::set<std::string>& allowed,
    const std::string& object_name,
    bool allow_unknown_fields) {
  if (allow_unknown_fields) return Status::Ok();
  for (const auto& [key, unused] : value.object_value) {
    (void)unused;
    if (allowed.find(key) == allowed.end()) {
      return Status(StatusCode::kInvalidArgument, "unknown key in " + object_name + ": " + key);
    }
  }
  return Status::Ok();
}

const JsonValue* FindField(const JsonValue& object, const std::string& key) {
  const auto found = object.object_value.find(key);
  if (found == object.object_value.end()) return nullptr;
  return &found->second;
}

Status RequireType(const JsonValue* value, JsonValue::Type type, const std::string& name) {
  if (value == nullptr) {
    return Status(StatusCode::kInvalidArgument, "missing required field: " + name);
  }
  if (value->type != type) {
    return Status(StatusCode::kInvalidArgument, "invalid type for field: " + name);
  }
  return Status::Ok();
}

Status NumberToUint64(double value, std::uint64_t& out, const std::string& name) {
  if (value < 0.0 || std::floor(value) != value) {
    return Status(StatusCode::kInvalidArgument, "field must be a non-negative integer: " + name);
  }
  out = static_cast<std::uint64_t>(value);
  return Status::Ok();
}

Status NumberToSize(double value, std::size_t& out, const std::string& name) {
  if (value < 0.0 || std::floor(value) != value) {
    return Status(StatusCode::kInvalidArgument, "field must be a non-negative integer: " + name);
  }
  out = static_cast<std::size_t>(value);
  return Status::Ok();
}

Status ReadOptionalString(
    const JsonValue& object,
    const std::string& key,
    std::string& out) {
  const JsonValue* value = FindField(object, key);
  if (value == nullptr) return Status::Ok();
  if (value->type != JsonValue::Type::kString) {
    return Status(StatusCode::kInvalidArgument, "invalid type for field: " + key);
  }
  out = value->string_value;
  return Status::Ok();
}

Status ReadOptionalNumber(
    const JsonValue& object,
    const std::string& key,
    double& out) {
  const JsonValue* value = FindField(object, key);
  if (value == nullptr) return Status::Ok();
  if (value->type != JsonValue::Type::kNumber) {
    return Status(StatusCode::kInvalidArgument, "invalid type for field: " + key);
  }
  out = value->number_value;
  return Status::Ok();
}

} // namespace riva
